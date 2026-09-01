import time
import wave
import numpy as np
import struct as st
import argparse
import os
import crc16 as crc
import pdb

header_fmt = 'HHHHHII'
header_nbyte = 20


class DataInterpreter:
    def __init__(self, verbose, exit_when_error):
        self.seq_num = np.zeros(100, dtype=np.int32)
        self.pos = 0
        self.verbose = verbose
        self.exit_when_error = exit_when_error

    def Interprete(self, dat):
        header = {}
        while self.pos + header_nbyte < len(dat):
            header_raw = st.unpack(
                header_fmt, dat[self.pos:self.pos + header_nbyte])
            header['magic'] = header_raw[0]
            header['ch'] = header_raw[1]
            header['seqn'] = header_raw[2]
            header['crc'] = header_raw[3]
            header['len'] = header_raw[4]
            header['timestamp'] = header_raw[5]
            header['tollen'] = header_raw[6]
            if header['magic'] != 0x2b5a:
                self.pos += 1
                continue
            self.pos += header_nbyte
            if self.pos + header['len'] > len(dat):
                if self.verbose >= 2:
                    print('error len')
                    print(header)
                self.pos += 1 - header_nbyte
                if self.exit_when_error:
                    break
                else:
                    continue
            raw_data = dat[self.pos:self.pos + header['len']]
            if crc.crc16xmodem(raw_data) != header['crc']:
                # pdb.set_trace()
                # crc error
                if self.verbose >= 2:
                    print(f'\ncurrent pos={self.pos}')
                    print('\ncrc error')
                    print(header)
                    print('crc should be %d' % crc.crc16xmodem(raw_data))
                self.pos += 1 - header_nbyte
                if self.exit_when_error:
                    break
                else:
                    continue
            if header['seqn'] != self.seq_num[header['ch']]:
                # lost data
                if self.verbose >= 2:
                    print('\nch %d lost data' % header['ch'])
                    print('current seq_num:%d' % self.seq_num[header['ch']])
                    print(header)
                self.seq_num[header['ch']] = header['seqn']
                if self.exit_when_error:
                    break

            self.seq_num[header['ch']] += 1
            self.pos += header['len']
            # if header['ch'] == 1:
            #     print(header)
            yield (header['ch'], header['tollen'], header['timestamp'], raw_data)

def dump_timestamp(timestamp_array,args):
    output_list = [os.path.join(args.opath, str(i) + '.timestamp')
                   for i in range(len(timestamp_array))]
    if args.v >= 1:
        print(output_list)
    for timestamp,of in zip(timestamp_array,output_list):
        timestamp = np.array(timestamp,dtype=np.float32)
        timestamp[:,0] -= timestamp[0,0]
        timestamp[:,0] *= args.timestamp_interval
        np.savetxt(of,timestamp,fmt='%f %d %d')

def dump_data(data_array, args):
    output_list = [os.path.join(args.opath, str(i) + '.raw')
                   for i in range(len(data_array))]
    if args.v >= 1:
        print(output_list)
    [np.array(data_array[i], dtype=np.uint8).tofile(output_list[i])
     for i in range(len(data_array))]
    if args.pack_as_wav:
        lenlist = [len(x) for x in data_array]
        minlen = min(lenlist)
        if lenlist.count(lenlist[0]) != len(lenlist):
            print(
                'length of data in each channel is not match,will cut as min length in all channel')
            print('length in each channel:')
            print(lenlist)
            data_array = [x[:minlen] for x in data_array]
        if lenlist[0] % (args.wav_bytewidth) != 0:
            print(
                'fail to pack all channel as a wav,because len of data is not divisible by wav_bytewidth')
        else:
            tmp = [np.array(x, np.uint8) for x in data_array]
            data_pack = np.stack(tmp, axis=0)
            zone = np.arange(args.wav_bytewidth)
            data_pack = data_pack.reshape([-1, minlen // args.wav_bytewidth,
                                           args.wav_bytewidth]).transpose([1, 0, 2]).astype(np.uint8)
            data_pack_raw = data_pack.tostring()
            t2 = time.time()
            # pass
            wav_file = os.path.join(args.opath, args.wav_name)
            with wave.open(wav_file, 'wb') as wav:
                wav.setframerate(args.wav_sr)
                wav.setnchannels(len(data_array))
                wav.setsampwidth(args.wav_bytewidth)
                wav.writeframes(data_pack_raw)
            pass


def JL_DataInterpreter(args):
    output_data = []
    [output_data.append([]) for i in range(100)]
    timestamp_data = []
    [timestamp_data.append([]) for i in range(100)]
    t1 = time.time()
    try:
        with open(args.i, 'rb') as f:
            dat = f.read()
            di = DataInterpreter(args.v, args.exit_when_error)
            cnt = 0
            for ch, tollen, timestamp, raw_data in di.Interprete(dat):
                cnt += 1
                if len(output_data[ch]) + len(raw_data) != tollen:
                    padding_len = tollen - len(raw_data) - len(output_data[ch])
                    if args.v >= 1:
                        print("#######padding %d data in channel:%d position:%d" %
                              (padding_len, ch, len(output_data[ch])))
                    output_data[ch] += np.uint8(np.zeros(padding_len, dtype=np.uint8) + args.lost_data_padding).tostring()
                output_data[ch] += raw_data
                timestamp_data[ch].append( [timestamp,tollen-len(raw_data),tollen] )
                # print('complete:%.02f%%' %
                #       (float(di.pos) / len(dat) * 100), end='\r')
    except Exception as e:
        print(e)
        print('except occur,ending interpreter')
    while [] in output_data:
        output_data.remove([])
    while [] in timestamp_data:
        timestamp_data.remove([])
    dump_data(output_data, args)
    dump_timestamp(timestamp_data, args)
    t2 = time.time()
    print('cost time %ds' % (t2 - t1))
    pass


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--opath',
        type=str,
        default='.',
        help='output path'
    )
    parser.add_argument(
        '--i',
        type=str,
        help='input file',
        default='out_test.raw'
    )
    parser.add_argument(
        '--v',
        type=int,
        default=2,
        help='verbose mode=[0 1 2]'
    )
    parser.add_argument(
        '--exit_when_error',
        type=int,
        default=1,
        help='stop interpreting data when error occur'
    )
    parser.add_argument(
        '--pack_as_wav',
        type=int,
        default=0,
        help='pack all channel data as pcm wave file'
    )
    parser.add_argument(
        '--wav_sr',
        type=int,
        default=16000,
        help='sample rate of pcm wave file'
    )
    parser.add_argument(
        '--wav_name',
        type=str,
        default='JL_DataInterpreter_WavOut.wav'
    )
    parser.add_argument(
        '--wav_bytewidth',
        type=int,
        default=2,
        help='bitwidth of pcm data'
    )
    parser.add_argument(
        '--lost_data_padding',
        type=int,
        default=0
    )
    parser.add_argument(
        '--timestamp_interval',
        type=float,
        default=1
    )

    args, _ = parser.parse_known_args()

    if args.pack_as_wav and (args.wav_sr == None or args.wav_name == None):
        print('need to set wav_name and wav_sr when pack_as_wav is used')
    else:
        JL_DataInterpreter(args)
