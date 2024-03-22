import argparse
import pandas as pd


def preprocess(verbose,
               input_filename,
               output_filename,
               min_elevation,
               max_elevation):
    """
    Preprocess a CSV file
    """

    if verbose:
        print(f'Reading {input_filename}')

    df = pd.read_csv(input_filename)

    total = len(df)

    if verbose:
        print(f'Read {total} photons')

    # Filter out photons
    df = df[df['geoid_corrected_h'] > min_elevation]

    if verbose:
        print(f'Filtered {total - len(df)} photons')

    df = df[df['geoid_corrected_h'] < max_elevation]

    if verbose:
        print(f'Filtered {total - len(df)} photons')

    if verbose:
        print(f'Writing {len (df)} to {output_filename}')

    df.to_csv(output_filename, index=False)


if __name__ == "__main__":
    """ Command line entry point """

    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', action="store_true", default=False)
    parser.add_argument('--min_elevation', default=-80)
    parser.add_argument('--max_elevation', default=20)
    parser.add_argument('input_filename')
    parser.add_argument('output_filename')
    args = parser.parse_args()

    preprocess(args.verbose,
               args.input_filename,
               args.output_filename,
               args.min_elevation,
               args.max_elevation)
