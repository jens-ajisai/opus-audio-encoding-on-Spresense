import asyncio
import logging
import platform
import sys
import socket

from serial_asyncio import open_serial_connection

logger = logging.getLogger(__name__)


async def main():
    # wait for the connection
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    addr = ("127.0.0.1", 20003)

    # open the serial port to receive stream from Spresense
    reader, writer = await open_serial_connection(url='/dev/tty.SLAB_USBtoUART', baudrate=115200, rtscts=True,dsrdtr=True)
    logger.info(f"opened serial")
    
    while True:
        # forward data
        data = await reader.read(1)
        client_socket.sendto(data, addr)
        logger.info(f"sent: {data}")
    client_socket.close()

if __name__ == "__main__":
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)-15s %(levelname)s: %(message)s",
    )
    logger.setLevel(logging.DEBUG)

    print("UDP server up and listening")

    asyncio.run(main())