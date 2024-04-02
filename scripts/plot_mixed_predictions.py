#!/usr/bin/env python3

import argparse
import numpy as np
import pandas as pd
import matplotlib as mpl
import matplotlib.pyplot as plt


def plot(df, col, ax, l, color):
    """
    Plot a subplot tile
    """

    df = df.sort_values(by=col)
    df.plot(ax=ax,
            subplots=True,
            x="Dataset",
            y=[col],
            kind="bar",
            rot=0,
            stacked=False,
            legend=None,
            color=color)
    u = np.mean(df[col])
    ax.hlines(y=u, xmin=-1, xmax=1000, linewidth=2, linestyles='-',
              color='black')
    ax.hlines(y=u, xmin=-1, xmax=1000, linewidth=3, linestyles=':',
              color=color)
    ax.annotate(f'average={u:0.2f}', xy=(2, u-0.05), xytext=(0, u-0.05))
    ax.set_ylim([0.0, 1.0])
    ax.set_ylabel(l)
    ax.set_title(None)
    ax.set_xlabel(None)
    ax.set_xticks([])
    ax.grid(axis='x')
    ax.set_yticks(np.arange(0.0, 1.1, 0.1))


def plot_mixed_predictions(verbose, title, input_fn, output_fn):
    """
    Create a scatter plot of mixed predictions
    """

    mpl.rcParams['figure.dpi'] = 300
    plt.style.use('fivethirtyeight')
    colors = plt.rcParams['axes.prop_cycle'].by_key()['color']

    df = pd.read_csv(input_fn)
    df['Dataset'] = df.index

    fig, axs = plt.subplots(2, 3, layout='constrained', figsize=[14, 7])

    plot(df, 'F1', axs[0][0],
         r'$F1$', colors[0])
    plot(df, 'F1_r0', axs[0][1],
         r'$F1_{calibrated}$', colors[1])
    plot(df, 'Wght_F1', axs[0][2],
         r'$Weighted\ F1$', colors[2])
    plot(df, 'Wght_F1_r0', axs[1][0],
         r'$Weighted\ F1_{calibrated}$', colors[3])
    plot(df, 'BA', axs[1][1],
         r'$Balanced\ Accuracy$', colors[4])
    plot(df, 'Acc', axs[1][2],
         r'$Acc$', colors[5])

    plt.title(title)
    plt.savefig(output_fn, bbox_inches='tight', transparent=True)


if __name__ == "__main__":
    """ Command line entry point """

    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', action="store_true", default=False)
    parser.add_argument('-t', '--title', default=None)
    parser.add_argument('input_fn')
    parser.add_argument('output_fn')
    args = parser.parse_args()

    plot_mixed_predictions(args.verbose,
                           args.title,
                           args.input_fn,
                           args.output_fn)
