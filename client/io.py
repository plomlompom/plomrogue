# This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
# or any later version. For details on its copyright, license, and warranties,
# see the file NOTICE in the root directory of the PlomRogue source package.


from client.config.io import io


def send(string):
    io["file_out"].write(string + "\n")
    io["file_out"].flush()
