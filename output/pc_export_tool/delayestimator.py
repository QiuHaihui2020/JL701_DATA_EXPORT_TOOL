import numpy as np
import argparse


def main(args):
    ref = np.fromfile(args.ref, dtype=np.int16).astype(np.float)
    echo = np.fromfile(args.echo, dtype=np.int16).astype(np.float)
    minlen = min([len(ref), len(echo)])
    if minlen != -1:
        minlen = min([minlen,args.len])
    ref = ref[:minlen]
    echo = echo[:minlen]
    corr = np.real(np.fft.ifft(np.conj(np.fft.fft(ref))*np.fft.fft(echo)))
    delay = np.argmax(corr)
    if delay > minlen//2:
        delay = delay - minlen
    print('delay is %d point' % delay)
    half_idx = minlen//2
    corr1 = np.real(np.fft.ifft(
        np.conj(np.fft.fft(ref[:half_idx]))*np.fft.fft(echo[:half_idx])))
    delay1 = np.argmax(corr)
    if delay1 > minlen//2:
        delay1 = delay1 - minlen
    corr2 = np.real(np.fft.ifft(
        np.conj(np.fft.fft(ref[half_idx:]))*np.fft.fft(echo[half_idx:])))
    delay2 = np.argmax(corr)
    if delay2 > minlen//2:
        delay2 = delay2 - minlen
    print('delay shift is +-%d point' % (delay1-delay2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--ref',
        type=str,
        default=[]
    )
    parser.add_argument(
        '--echo',
        type=str,
        default=[]
    )
    parser.add_argument(
        '--len',
        type=int,
        default=-1
    )
    FLAGS, _ = parser.parse_known_args()
    main(FLAGS)
