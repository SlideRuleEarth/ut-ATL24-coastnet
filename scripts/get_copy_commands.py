import argparse
import glob
import random
import math
import sys

# Parse command line
parser = argparse.ArgumentParser()
parser.add_argument("--random_seed", help="RNG seed", type=int, default=123)
parser.add_argument("--fold", help="fold number", type=int)
parser.add_argument("--folds", help="total number of folds", type=int)
parser.add_argument("--training_dir", help="name of training file directory")
parser.add_argument("--testing_dir", help="name of testing file directory")
parser.add_argument("--filename_glob", help="glob of all filenames")
args = parser.parse_args()
print(args, file=sys.stderr)

# Check args
if args.folds == 0:
    raise ValueError("'folds' cannot be 0")

if args.fold >= args.folds:
    raise ValueError("'fold' must be less than 'folds'")

# Get filenames
fns = glob.glob(args.filename_glob)
total = len(fns)
print(f'Total files = {total}', file=sys.stderr)

if total < args.folds:
    raise ValueError("The number of files must be >= 'folds'")

# Random their order
random.seed(args.random_seed)
random.shuffle(fns)

# Split into training/test groups
files_per_fold = math.ceil(total/args.folds)
print(f'files_per_fold = {files_per_fold}', file=sys.stderr)

# Generate the copy commands for this fold
for n, fn in enumerate(fns):
    if n // files_per_fold == args.fold:
        print(f'cp {fn} {args.testing_dir}')
    else:
        print(f'cp {fn} {args.training_dir}')
