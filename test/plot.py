import pandas as pd
import matplotlib.pyplot as plt

# Load CSV
df = pd.read_csv('stress_test_results.csv')

# Plot data
plt.figure(figsize=(12, 6))
plt.plot(df['timestamp'], df['client_count'], label='Client Count', color='blue')
plt.plot(df['timestamp'], df['client_errors'], label='Client Errors', color='red')
plt.plot(df['timestamp'], df['messages_sent'], label='Messages Sent', color='green')
plt.plot(df['timestamp'], df['messages_received'], label='Messages Received', color='orange')
plt.plot(df['timestamp'], df['batches_received'], label='Batches Received', color='purple')

# Labels and title
plt.xlabel('Time (ms)')
plt.ylabel('Count')
plt.title('WebSocket Stress Test Metrics Over Time')
plt.legend()
plt.grid(True)
plt.tight_layout()

# Show plot
plt.show()
