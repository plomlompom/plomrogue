from client.config.io import io

def send(string):
    io["file_out"].write(string + "\n")
    io["file_out"].flush()
