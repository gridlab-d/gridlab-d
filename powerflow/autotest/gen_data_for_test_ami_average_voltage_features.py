"""Use this script to generate player data as well as expected results
for "test_ami_average_voltage_features_NR.glm."

This script was run with Python 3.8.6. Third party package versions
are noted when imported. This all could be accomplished with the
standard library (i.e., without 3rd party packages like numpy and
pandas), but the objective was speed of development :)

It's worth noting that if you have the latest Windows update as of
2020-11-25, Windows itself completely broke numpy/pandas, and this is
not expected to be resolved until January 2021. Nice going, Microsoft.
https://developercommunity.visualstudio.com/content/problem/1207405/fmod-after-an-update-to-windows-2004-is-causing-a.html
"""

# Standard library:
from datetime import datetime, timedelta
import cmath

# Third party:
import numpy as np      # version 1.19.4
import pandas as pd     # version 1.1.4

# Constants:
START = datetime(year=2012, month=1, day=1, hour=0, minute=0, second=0)
STOP = START + timedelta(hours=1)
INTERVAL = 15   # minutes
# Hard-code our average blocks in per unit. We want different average
# for the different phases in order to make the test more rigorous.
PHASES = ['A', 'B', 'C']
AVERAGES = pd.DataFrame(
    [[1.05, 1.0, 0.95],
     [1.0, 0.95, 0.9],
     [0.95, 0.9, 1.05],
     [0.9, 1.05, 1.0]],
    columns=PHASES,
    index=pd.date_range(
        start=START, end=STOP, freq=f'{INTERVAL}min', closed='right')
)

# Use a date format GridLAB-D likes.
DATE_FORMAT = '%Y-%m-%d %H:%M:%S'

# Define nominal voltages
V_NOM = 7200
V_A = cmath.rect(V_NOM, 0)
V_B = cmath.rect(V_NOM, 120 * cmath.pi / 180)
V_C = cmath.rect(V_NOM, -120 * cmath.pi / 180)


def main():
    """Generate .csv and .player files for the test."""

    # Whip up our date range for the full test.
    dr = pd.date_range(start=START, end=STOP, freq='1min', closed='left')

    # Create a dataframe filled with nominal voltages
    df = pd.DataFrame([[V_A, V_B, V_C]], columns=PHASES,
                      index=dr)

    # Initialize a random number generator.
    rng = np.random.default_rng(seed=10)

    # Randomly perturb each element of the DataFrame.
    perturb = rng.uniform(low=0.9, high=1.1, size=df.shape)
    df *= perturb

    # Resample and take the mean for each interval.
    # Pandas does not let us do this for complex numbers:
    # resampled = df.resample('15min').mean()

    # So, we loop.
    idx = []
    for i_averages, i_df in enumerate(range(0, df.shape[0], INTERVAL)):
        # Extract data for this time range.
        subset = df.iloc[i_df:i_df+INTERVAL]

        # Compute the scaling factor to get the desired mean.
        factors = AVERAGES.iloc[i_averages, :] / (subset.mean().abs() / V_NOM)

        # Scale the subset of data so that we get the desired averages.
        df.iloc[i_df:i_df+INTERVAL] = df.iloc[i_df:i_df+INTERVAL] * factors

        # To avoid hard-coding (though one must wonder what's the point
        # with some of the other hard-coding going on...), track the
        # timestamp corresponding to this subset of data.
        idx.append(subset.index[-1] + timedelta(minutes=1))

    # Our DataFrame now contains the data we want for our player files.
    for phase in df.columns:
        # First, get the output as a string. This is not an efficient
        # way to do this, but Python likes to output complex numbers
        # within parentheses.
        out_string = df[phase].to_csv(
            path_or_buf=None, date_format=DATE_FORMAT, header=False)

        # Get rid of those pesky parentheses.
        out_string = out_string.replace('(', '').replace(')', '')

        # Write to file.
        with open(f'data_voltage_values_AMI_average_test_{phase}.player',
                  'w') as f:
            f.write(out_string)

    # Convert expected per unit averages to volts.
    expected = AVERAGES * pd.DataFrame(
        [[V_A, V_B, V_C]], columns=PHASES,
        index=AVERAGES.index).abs()

    # The first row should be 0's, because averages in GridLAB-D are
    # initialized to be 0 until the first stats interval is complete.
    expected.loc[START, :] = 0.0
    # Now we need to sort.
    expected.sort_index(inplace=True)

    # Write expected averages to files.
    for phase in expected.columns:
        expected[phase].to_csv(
            f'data_AMI_average_test_{INTERVAL}_min_avg_voltage_mag_{phase}.csv',
            header=False, date_format=DATE_FORMAT
        )

    pass


if __name__ == '__main__':
    main()
