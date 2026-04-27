import argparse
import os
from pydub import AudioSegment

# Dependencies: 
# pydub:  pip install pydub
# ffmpeg: brew install ffmpeg




def cut_audio():
    # 1. Setup Argument Parser
    parser = argparse.ArgumentParser(description="A simple CLI tool to cut audio files.")
    
    parser.add_argument("-s", "--source", required=True, help="Source audio file name")
    parser.add_argument("-ts", "--start", type=float, default=0.0, help="Start time in seconds")
    parser.add_argument("-te", "--end", type=float, help="End time in seconds (overrides duration)")
    parser.add_argument("-du", "--duration", type=float, help="Duration to cut starting from ts")
    parser.add_argument("-o", "--output", required=True, help="Output file name")
    parser.add_argument("-f", "--format", help="Output format (mp3 or wav). Defaults to source format.")

    args = parser.parse_args()

    # 2. Load the source file
    if not os.path.exists(args.source):
        print(f"Error: File '{args.source}' not found.")
        return

    print(f"Loading {args.source}...")
    audio = AudioSegment.from_file(args.source)

    # 3. Calculate Cut Points (Pydub works in milliseconds)
    start_ms = args.start * 1000
    
    if args.end is not None:
        end_ms = args.end * 1000
    elif args.duration is not None:
        end_ms = start_ms + (args.duration * 1000)
    else:
        end_ms = len(audio) # Default to end of file if nothing else is provided

    # 4. Perform the cut
    cut_audio = audio[start_ms:end_ms]

    # 5. Determine Format and Export
    out_format = args.format if args.format else args.source.split('.')[-1]
    
    print(f"Exporting to {args.output} as {out_format}...")
    cut_audio.export(args.output, format=out_format)
    print("Done!")

if __name__ == "__main__":
    cut_audio()