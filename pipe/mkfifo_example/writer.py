import os
import time

# Path to the named pipe
PIPE_PATH = "message_pipe"

print(f"Writer process (PID: {os.getpid()})")
print(f"Opening pipe for writing...")

# Open the pipe in write mode
with open(PIPE_PATH, 'w') as pipe:
    for i in range(5):
        message = f"Message #{i} from writer"
        print(f"Sending: {message}")
        pipe.write(message + '\n')
        pipe.flush()  # Make sure the message is sent immediately
        time.sleep(1)  # Wait for 1 second between messages

print("Writer finished")