import os

# Path to the named pipe
PIPE_PATH = "message_pipe"

print(f"Reader process (PID: {os.getpid()})")
print(f"Opening pipe for reading...")

# Open the pipe in read mode
with open(PIPE_PATH, 'r') as pipe:
    while True:
        message = pipe.readline().strip()
        if not message:
            break
        print(f"Received: {message}")

print("Reader finished")