import pandas as pd
import matplotlib.pyplot as plt
import re

"""
--- COLOCATED ---
Samples: 10000
Mean RTT: 21.91 μs
Std Dev: 5.37 μs

--- TCP ---
Samples: 10000
Mean RTT: 465.65 μs
Std Dev: 143.54 μs
"""

# === CONFIG ===
FILE_INFO = [
    ("round_trip_log_colocated_multiple_msg.csv", "colocated"),
    ("round_trip_log_multiple_msg.csv", "tcp")
]
SAVE_PLOTS = True
SHOW_PLOTS = True
DOWNSAMPLE_FACTOR = 10

# === Function to extract RTT
def extract_rtt_us(metric_str):
    match = re.search(r'RTT = (\d+)', str(metric_str))
    return int(match.group(1)) if match else None

# === Load and label each dataset ===
latencies_by_mode = {}

for filepath, mode in FILE_INFO:
    df = pd.read_csv(filepath)
    df["RTT_us"] = df["Metric"].apply(extract_rtt_us)
    latencies = df["RTT_us"].dropna()
    latencies_by_mode[mode] = latencies

    # Print stats
    print(f"--- {mode.upper()} ---")
    print(f"Samples: {len(latencies)}")
    print(f"Mean RTT: {latencies.mean():.2f} μs")
    print(f"Std Dev: {latencies.std():.2f} μs\n")

# === HISTOGRAM ===
plt.figure(figsize=(10, 6))

# Set consistent bins across all modes
bin_edges = range(0, 1001, 25)  # bins every 25 μs from 0 to 1000

plt.xlim(0, 1000)

for mode, data in latencies_by_mode.items():
    plt.hist(data, bins=bin_edges, alpha=0.6, edgecolor='black', label=mode)

plt.title("Histogram of RTT (μs)")
plt.xlabel("Round-trip latency (μs)")
plt.ylabel("Frequency")
plt.legend()

if SAVE_PLOTS:
    plt.savefig("plots/comparison_histogram_log_rtt.png", dpi=150)
if SHOW_PLOTS:
    plt.show()


# === TIME SERIES ===
plt.figure(figsize=(10, 6))
plt.yscale("log")
for mode, data in latencies_by_mode.items():
    downsampled = data.iloc[::DOWNSAMPLE_FACTOR]
    plt.plot(downsampled.values, label=f"{mode} (1/{DOWNSAMPLE_FACTOR})", marker=None, linewidth=1)
plt.title("RTT Time Series (Downsampled)")
plt.xlabel("Sample Index")
plt.ylabel("RTT (μs)")
plt.legend()
if SAVE_PLOTS: plt.savefig("plots/comparison_log_timeseries_rtt.png", dpi=150)
# if SHOW_PLOTS: plt.show()
