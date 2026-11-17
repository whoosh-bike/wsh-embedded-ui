import argparse
import io
import json
import logging
import os
import shutil
import sys

import numpy as np
from natsort import natsorted
from PIL import Image

# python3 utils/convert-graphics.py -dw 128 -dh 64 -s 'RGB' -a -c -o

IMG_COLOR_SCHEME_GRAY = 0
IMG_COLOR_SCHEME_RGB = 1

# WeUi_Gfx_Storage_t.Name is char[32]: the firmware rejects a longer name
ASSET_NAME_MAX_LEN = 31


class CustomFormatter(logging.Formatter):
    grey = "\x1b[38;20m"
    yellow = "\x1b[33;20m"
    red = "\x1b[31;20m"
    blue = "\x1b[36;20m"
    bold_red = "\x1b[31;1m"
    reset = "\x1b[0m"
    format = "%(asctime)s - %(levelname)s  \t (%(filename)s:%(lineno)d): %(message)s"

    FORMATS = {
        logging.DEBUG: blue + format + reset,
        logging.INFO: grey + format + reset,
        logging.WARNING: yellow + format + reset,
        logging.ERROR: red + format + reset,
        logging.CRITICAL: bold_red + format + reset,
    }

    def format(self, record):
        log_fmt = self.FORMATS.get(record.levelno)
        formatter = logging.Formatter(log_fmt)
        return formatter.format(record)


class GraphicsConverter:
    def __init__(self, width: int, height: int, color_scheme: int, alpha: bool, **kwargs) -> None:
        self.script_abs_path = os.path.dirname(os.path.abspath(__file__))

        self.k_name = "name"
        self.k_type = "type"
        self.k_comp = "comp"
        self.k_width = "widt"
        self.k_height = "heig"
        self.k_size = "size"
        self.color_scheme = color_scheme
        self.alpha = alpha

        self.allowed_extensions = [".bmp", ".png", ".jpg", ".jpeg"]

        self.images_src_path = self.script_abs_path + "/src"
        logger.info(f"Image src path: {self.images_src_path}")

        self.images_dst_path = self.script_abs_path + "/dst"
        logger.info(f"Image dst path: {self.images_dst_path}")

        if os.path.exists(self.images_dst_path):
            shutil.rmtree(self.images_dst_path)
        os.makedirs(self.images_dst_path)

        self.DISPLAY_WIDTH = width
        self.DISPLAY_HEIGHT = height

    def find_files_in(self, path: str) -> list:
        images_list = []
        for file_name in os.listdir(path):
            file_path = os.path.join(path, file_name)
            if os.path.isfile(file_path):
                file_extension = os.path.splitext(file_name)[1].lower()
                if file_extension in self.allowed_extensions:
                    images_list.append(file_name)
                else:
                    logger.warning(
                        f'File "{file_name}" extention is not in {
                        self.allowed_extensions}'
                    )
                    continue
            else:
                logger.warning(f'"{file_name}" is not a file')

        logger.info(f"Found {len(images_list)} valid files")
        return natsorted(images_list)

    @staticmethod
    def calculate_luma(r, g, b):
        return int(0.2126 * r + 0.7152 * g + 0.0722 * b)

    def convert_images(self, files: list) -> None:
        total_size = 0

        for file in files:
            name = file.split(".")
            name.pop(-1)
            name = "_".join(name)
            name = name.replace(" ", "_").replace("-", "_")
            if len(name.encode("utf8")) > ASSET_NAME_MAX_LEN:
                logger.error(f'Name "{name}" is longer than {ASSET_NAME_MAX_LEN} bytes, skipped')
                continue
            logger.info(f'File "{name}" convertation..')

            image = Image.open(self.images_src_path + "/" + file)
            logger.info(f"Source image mode: {image.mode}")

            if image.size[0] > self.DISPLAY_WIDTH or image.size[1] > self.DISPLAY_HEIGHT:
                logger.warning(f"Width {image.size[0]} or height {image.size[1]} is not valid!")

            # image = image.resize(
            #     (self.DISPLAY_WIDTH, self.DISPLAY_HEIGHT))
            # logger.info(f"Resized image to {self.DISPLAY_WIDTH}x{
            #             self.DISPLAY_HEIGHT}")

            image = image.convert("RGBA")
            image_rgba = np.array(image).flatten().tolist()
            image_buff = []

            if self.color_scheme == IMG_COLOR_SCHEME_GRAY:
                data_gray_a = []
                for i in range(0, len(image_rgba), 4):
                    # "Luma" (ITU-R BT.709) RGB -> gray convertation
                    gray = self.calculate_luma(
                        image_rgba[0 + i], image_rgba[1 + i], image_rgba[2 + i]
                    )
                    data_gray_a.append(gray)
                    if self.alpha:
                        data_gray_a.append(image_rgba[3 + i])
                    # logger.debug(
                    #     f'R {image_rgba[0]}, G {image_rgba[1]} B {image_rgba[2]} => GR {gray}')
                image_buff = bytes(data_gray_a)
            elif self.color_scheme == IMG_COLOR_SCHEME_RGB:
                data_rgb_a = []
                for i in range(0, len(image_rgba), 4):
                    r, g, b, a = image_rgba[i : i + 4]
                    data_rgb_a.extend([r, g, b])
                    if self.alpha:
                        data_rgb_a.append(a)
                image_buff = bytes(data_rgb_a)

            image_info = {
                self.k_name: name,
                "type": "img",
                "comp": "false",
                "widt": image.size[0],
                "heig": image.size[1],
                "size": len(image_buff),
                "scheme": self.color_scheme,
                "alpha": int(self.alpha),
            }

            out_io = io.BytesIO()
            out_io.write(bytes(json.dumps(image_info), encoding="utf8") + image_buff)
            out_file_size_bytes = len(out_io.getvalue())
            total_size += out_file_size_bytes

            logger.info(f"\tName: {image_info[self.k_name]}")
            logger.info(f"\tComp: {image_info[self.k_comp]}")
            logger.info(
                f"\tType: {image_info[self.k_type]}, Size: {
                image_info[self.k_size]}, Color scheme: {self.color_scheme}, Alpha channel: {self.alpha}"
            )
            logger.info(
                f"\tWith: {image_info[self.k_width]}, Height: {
                image_info[self.k_height]}"
            )
            logger.info(f"\tFile size: {out_file_size_bytes / 1024} kBytes")
            logger.info(f"\tJSON: {json.dumps(image_info)}\n")

            out_io.seek(0)
            with open(self.images_dst_path + "/" + name, "wb") as file:
                file.write(out_io.getvalue())

        logger.info(f"Total size: {total_size / 1024} kBytes\n")

    def create_c_file(self):
        images = natsorted(os.listdir(self.images_dst_path))
        out_path = self.script_abs_path + "/" + "gr_const_images.c"
        with open(out_path, "w") as c_file:
            c_file.write('#include "weui_gfx_types.h"\n')
            c_file.write('#include "platform.h"\n\n')

            for file_name in images:
                c_file.write(f"const u8 PL_GRAPHICS_STORAGE gr_img_{file_name}[] = {{\n")
                c_file.write(f'#embed "dst/{file_name}"\n')
                c_file.write("};\n\n")
                logger.info(f"Added #embed for {file_name}")

        logger.info(f"Generated {out_path}")


logger = logging.getLogger()
logger.setLevel(logging.DEBUG)

ch = logging.StreamHandler()
ch.setLevel(logging.INFO)
ch.setFormatter(CustomFormatter())

logger.addHandler(ch)

if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument("-c", "--convert", action="store_true", help="convert availible images")
    arg_parser.add_argument(
        "-o", "--output", action="store_true", help="code output for using in .c files"
    )
    arg_parser.add_argument("-dw", "--width", type=int, required=True, help="set display width")
    arg_parser.add_argument("-dh", "--height", type=int, required=True, help="set display height")
    arg_parser.add_argument(
        "-s",
        "--scheme",
        type=str,
        choices=["GRAY", "RGB"],
        required=True,
        help="Set color scheme (GRAY, RGB)",
    )
    arg_parser.add_argument("-a", "--alpha", action="store_true", help="Use aplha channel")
    cli_args = arg_parser.parse_args(sys.argv[1:])

    color_scheme = IMG_COLOR_SCHEME_GRAY
    if cli_args.scheme == "RGB":
        color_scheme = IMG_COLOR_SCHEME_RGB

    gr_converter = GraphicsConverter(cli_args.width, cli_args.height, color_scheme, cli_args.alpha)
    images = gr_converter.find_files_in(gr_converter.images_src_path)

    if cli_args.convert:
        gr_converter.convert_images(images)

    if cli_args.output:
        gr_converter.create_c_file()
