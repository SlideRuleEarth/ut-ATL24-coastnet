import argparse
import numpy as np
import pandas as pd
import plotly.express as px
import streamlit as st


st.set_page_config(
        page_title="ICESAT-2 Classification Viewer",
        page_icon=":earth_americas:")


def main(args):

    # Show args
    if args.verbose:
        print('input_filename:', args.input_filename)

    df = pd.read_csv(args.input_filename)

    if args.verbose:
        print(df)

    st.header(args.input_filename)

    minx = np.amin(df.along_track_dist)
    maxx = np.amax(df.along_track_dist)
    lenx = maxx - minx
    total_segments = int(lenx)//args.segment_size + 1
    segment_numbers = range(0, total_segments)
    segment_number = st.selectbox('Segment', segment_numbers)

    if args.verbose:
        print('minx', maxx)
        print('maxx', maxx)
        print('lenx', lenx)
        print('total_segments', total_segments)
        print('segment_numbers', segment_numbers)
        print('segment_number', segment_number)
        print('labels', set(df.manual_label))

    # Use discrete colormap
    df["manual_label"] = df["manual_label"].astype(str)

    x1 = minx + segment_number*args.segment_size
    x2 = minx + (segment_number + 1)*args.segment_size
    st.text(f'start x {x1}, end x {x2}')
    df2 = df[(df.along_track_dist > x1) & (df.along_track_dist < x2)]

    # This legend is for ATL24
    cdm = {'0': 'darkgray',
           '40': 'magenta',
           '41': 'cyan',
           '45': 'blue'}
    label_names = {'0': 'other',
                   '40': 'bathymetry',
                   '41': 'sea surface',
                   '45': 'water column'}

    fig = px.scatter(df2, x="along_track_dist", y="egm08_orthometric_height",
                     color='manual_label',
                     color_discrete_map=cdm,
                     hover_data=["along_track_dist",
                                 "egm08_orthometric_height"],
                     labels={'along_track_dist': 'Along-track distance (m)',
                             'egm08_orthometric_height': 'Elevation (m)'})
    fig.update_layout(template='simple_white')
    fig.update_scenes(aspectmode='data')
    fig.update_traces(marker=dict(opacity=1.0, line=dict(width=0)))
    fig.update_traces(marker_size=3)
    fig.for_each_trace(lambda t: t.update(name=label_names[t.name]))
    fig.update_layout(legend_title_text='Classification')
    st.plotly_chart(fig, use_container_width=True)

    fig = px.scatter(df2, x="along_track_dist", y="egm08_orthometric_height",
                     color='manual_label',
                     color_discrete_map=cdm,
                     hover_data=["along_track_dist",
                                 "egm08_orthometric_height"],
                     labels={'along_track_dist': 'Along-track distance (m)',
                             'egm08_orthometric_height': 'Elevation (m)'})
    fig.update_yaxes(scaleanchor="x", scaleratio=1)
    fig.update_layout(template='simple_white')
    fig.update_scenes(aspectmode='data')
    fig.update_traces(marker=dict(opacity=1.0, line=dict(width=0)))
    fig.update_traces(marker_size=3)
    fig.for_each_trace(lambda t: t.update(name=label_names[t.name]))
    fig.update_layout(legend_title_text='Classification')
    st.plotly_chart(fig, use_container_width=True)


if __name__ == "__main__":

    parser = argparse.ArgumentParser(
        prog='viewer',
        description='Interactive SCuBA elevation viewer')
    parser.add_argument(
        'input_filename',
        help="Input filename specification")
    parser.add_argument(
        '-s', '--segment_size', type=int, default=10000,
        help='Size in meters of one segment')
    parser.add_argument(
        '-v', '--verbose', action='store_true',
        help="Show verbose output")
    args = parser.parse_args()

    main(args)
