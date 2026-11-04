#!/usr/bin/env python3
"""Windows regression runner. Runs isolated copies of 3 official USGS test models.
Requires only Python 3 and the Stage 3 executable (no numpy).
"""
import argparse, pathlib, tempfile, subprocess, shutil, struct, math, sys, re, json
BASE=pathlib.Path(__file__).resolve().parent
MODELS=[('01A_nestedgrid_nognc','flow.nam','flow.hds'),('01B_nestedgrid_gnc','flow.nam','flow.hds'),('03_conduit_confined','ex3.nam','ex3.hds')]
HEADER=struct.Struct('<iiff16siii')

def read_hds(path):
    data=path.read_bytes(); pos=0; records=[]
    while pos < len(data):
        # Header (44 bytes); GNU sequential-unformatted adds a 4-byte record marker.
        wrapped=False
        if pos+4<=len(data) and struct.unpack_from('<i',data,pos)[0]==HEADER.size:
            wrapped=True; pos+=4
        if pos+HEADER.size>len(data):raise ValueError(f'truncated HDS header at {pos}')
        kstp,kper,pertim,totim,text,ncol,nrow,ilay=HEADER.unpack_from(data,pos)
        if not (1<=kstp<=100000 and 1<=kper<=100000 and 0<ncol*nrow<100000000):
            raise ValueError(f'bad HDS header at {pos}: {kstp=} {kper=} {ncol=} {nrow=}')
        pos+=HEADER.size
        if wrapped:
            if struct.unpack_from('<i',data,pos)[0]!=HEADER.size:raise ValueError('bad header record trailer')
            pos+=4
        size=4*ncol*nrow
        if pos+4<=len(data) and struct.unpack_from('<i',data,pos)[0]==size and wrapped:
            pos+=4; has_data_mark=True
        else:has_data_mark=False
        if pos+size>len(data): raise ValueError('truncated HDS values')
        vals=struct.unpack_from(f'<{ncol*nrow}f',data,pos); pos+=size
        if has_data_mark:
            if struct.unpack_from('<i',data,pos)[0]!=size:raise ValueError('bad data record trailer')
            pos+=4
        records.append(((kstp,kper,ilay,round(totim,5)),vals))
    return records

def make_sms(src,dest):
    # Keep source options and first six numeric fields; switch LINMETH to 3
    # For xMD input change the following solver-specific record to PCGU format.
    raw=src.read_text().splitlines(); numeric=[]
    for line in raw:
        st=line.strip()
        if not st or st.startswith(('#','!')): continue
        toks=st.split()
        try:
            [float(v.replace('D','E')) for v in toks[:7]]
            if len(toks)>=7:
                numeric=toks[:7];break
        except ValueError:pass
    if len(numeric)!=7: raise ValueError(f'Cannot parse SMS header {src}')
    numeric[-1]='3'
    # Use explicit settings, retaining the first 7 fields of the original.
    nonlin=int(float(numeric[5])); pieces=['# Release validation: GMRES+BoomerAMG (LINMETH=3)', ' '.join(numeric)]
    if nonlin!=0:pieces.append('0.9 0.07 0.1 0.0 200 1.1 0.2 1.0')
    pieces.append('BCGS 3 0 0 0.1 0.97')
    dest.write_text('\n'.join(pieces)+'\n')

def valid(a):return math.isfinite(a) and abs(a)<1e20

def compare(a,b,tol):
    if len(a)!=len(b):raise ValueError(f'head record count mismatch: {len(a)} vs {len(b)}')
    n=0;maxdiff=0.;ssq=0.;abssum=0.;excluded=0
    for (key1,va),(key2,vb) in zip(a,b):
        if key1!=key2 or len(va)!=len(vb):raise ValueError(f'head record mismatch: {key1} vs {key2}')
        for x,y in zip(va,vb):
            if not valid(x) or not valid(y):
                if (valid(x)!=valid(y)) or (x!=y): raise ValueError(f'Invalid/dry head mismatch: {key1}: {x} vs {y}')
                excluded+=1;continue
            d=abs(x-y);maxdiff=max(maxdiff,d);ssq+=d*d;abssum+=d;n+=1
    if not n:raise ValueError('No finite heads')
    return {'records':len(a),'compared':n,'excluded':excluded,'mae_m':abssum/n,'rmse_m':math.sqrt(ssq/n),'maxdiff_m':maxdiff,'pass':maxdiff<=tol}

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--exe',type=pathlib.Path,required=True);ap.add_argument('--max-head-diff',type=float,default=0.10);ap.add_argument('--json',type=pathlib.Path)
    args=ap.parse_args();exe=args.exe.resolve(); summaries=[]
    if not exe.exists():ap.error(f'EXE not found: {exe}')
    with tempfile.TemporaryDirectory(prefix='mfusg-amg-') as tmp:
        for name,nam,hds in MODELS:
            src=BASE/'fixtures'/name;dest=pathlib.Path(tmp)/name;shutil.copytree(src,dest)
            original=dest/'output'/hds
            reference=read_hds(original)
            shutil.rmtree(dest/'output')
            (dest/'output').mkdir()
            sms=next(dest.glob('*.sms'));make_sms(sms,sms)
            with (dest/'console.log').open('w') as log:
                p=subprocess.run([str(exe),nam],cwd=dest,stdout=log,stderr=subprocess.STDOUT,timeout=180)
            generated=dest/'output'/hds
            if not generated.exists():
                alt=dest/hds
                if alt.exists():generated=alt
            listing='\n'.join(x.read_text(errors='replace')[-4000:] for x in dest.rglob('*.lst'))
            console=(dest/'console.log').read_text(errors='replace')
            ok=p.returncode==0 and 'Normal termination of simulation' in console and generated.exists()
            record={'test':name,'exit_code':p.returncode,'normal_termination': 'Normal termination of simulation' in console,'head_output_found':generated.exists()}
            if ok:
                try:record.update(compare(reference,read_hds(generated),args.max_head_diff));ok=record['pass']
                except Exception as e:record['error']=str(e);ok=False
            else:record['console_tail']=console[-700:]
            record['pass']=ok;summaries.append(record)
            print(f"{'PASS' if ok else 'FAIL'} {name}: {record}")
    if args.json:args.json.write_text(json.dumps(summaries,indent=2)+'\n')
    return 0 if all(x['pass'] for x in summaries) else 1
if __name__=='__main__':sys.exit(main())
