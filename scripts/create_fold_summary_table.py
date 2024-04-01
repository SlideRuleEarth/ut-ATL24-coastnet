#!/usr/bin/env python3

import argparse
import pandas as pd
import matplotlib as mpl
import matplotlib.pyplot as plt


def create_fold_summary_table(verbose, title, input_fn, output_fn):
    """
    Create a table of fold summaries
    """

    mpl.rcParams['figure.dpi'] = 300
    plt.style.use('fivethirtyeight')

    df = pd.read_csv(input_fn).astype('object')
    df.update(df[['Acc']].map('{:,.3f}'.format))
    df.update(df[['F1']].map('{:,.3f}'.format))
    df.update(df[['F1_r0']].map('{:,.3f}'.format))
    df.update(df[['Wght_F1']].map('{:,.3f}'.format))
    df.update(df[['Wght_F1_r0']].map('{:,.3f}'.format))
    df.update(df[['BA']].map('{:,.3f}'.format))
    print(df)

    fig, ax = plt.subplots(figsize=[6, 2])
    fig.patch.set_visible(False)
    ax.axis('off')
    ax.axis('tight')
    ax.set_title(title, fontsize=10)
    table = ax.table(cellText=df.values,
                     colLabels=[r'$Fold$',
                                r'$Accuracy$',
                                r'$F1$',
                                r'$F1_{calibrated}$',
                                r'$Weighted\ F1$',
                                r'$Weighted\ F1_{calibrated}$',
                                r'$Balanced\ Accuracy$'],
                     loc='center',
                     cellLoc='center')
    table.auto_set_font_size(False)
    table.set_fontsize(9)
    table.auto_set_column_width(col=list(range(len(df.columns))))

    fig.tight_layout()
    plt.savefig(output_fn, bbox_inches='tight')


if __name__ == "__main__":
    """ Command line entry point """

    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', action="store_true", default=False)
    parser.add_argument('-t', '--title', default=None)
    parser.add_argument('input_fn')
    parser.add_argument('output_fn')
    args = parser.parse_args()

    create_fold_summary_table(args.verbose,
                              args.title,
                              args.input_fn,
                              args.output_fn)
