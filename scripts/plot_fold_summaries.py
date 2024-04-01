#!/usr/bin/env python3

import argparse
import pandas as pd
import matplotlib.pyplot as plt


def plot_fold_summaries(verbose, title, input_fn, output_fn):
    """
    Create a bar chart of fold summaries
    """

    plt.style.use('fivethirtyeight')
    df = pd.read_csv(input_fn)
    print(df)
    fig = plt.figure()
    ax = fig.add_subplot(211)
    ax = df.plot(x="Fold",
                 y=["F1",
                    "F1_r0",
                    "Wght_F1",
                    "Wght_F1_r0",
                    "BA"],
                 kind="bar",
                 rot=0,
                 stacked=False)
    ax.set_ylim([0.0, 1.0])
    ax.set_ylabel('score')
    ax.set_xlabel('fold number')
    ax.legend(bbox_to_anchor=(1, 1.02),
              labels=[r'$F1$',
                      r'$F1_{calibrated}$',
                      r'$Weighted\ F1$',
                      r'$Weighted\ F1_{calibrated}$',
                      r'$Balanced\ Accuracy$'],
              loc='upper left')

    plt.title(title)
    plt.savefig(output_fn, bbox_inches='tight')


if __name__ == "__main__":
    """ Command line entry point """

    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', action="store_true", default=False)
    parser.add_argument('-t', '--title', default=None)
    parser.add_argument('input_fn')
    parser.add_argument('output_fn')
    args = parser.parse_args()

    plot_fold_summaries(args.verbose,
                        args.title,
                        args.input_fn,
                        args.output_fn)
