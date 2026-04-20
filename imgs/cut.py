import os


def extract_rgb_frames(
    input_path: str,
    output_path: str,
    width: int,
    height: int,
    start_frame: int,
    end_frame: int,
) -> None:
    if start_frame < 0 or end_frame < start_frame:
        raise ValueError("Invalid frame range.")

    frame_size = width * height * 3
    file_size = os.path.getsize(input_path)

    if file_size % frame_size != 0:
        raise ValueError("Input file size is not a multiple of frame size.")

    total_frames = file_size // frame_size

    if end_frame >= total_frames:
        raise ValueError(
            f"Frame range out of bounds. Total frames: {total_frames}, "
            f"requested: {start_frame}-{end_frame}"
        )

    num_frames = end_frame - start_frame + 1
    start_offset = start_frame * frame_size
    bytes_to_read = num_frames * frame_size

    with open(input_path, "rb") as fin:
        fin.seek(start_offset)
        data = fin.read(bytes_to_read)

    with open(output_path, "wb") as fout:
        fout.write(data)

    print(f"Done.")
    print(f"Input file:  {input_path}")
    print(f"Output file: {output_path}")
    print(f"Frame size:  {frame_size} bytes")
    print(f"Total frames in input: {total_frames}")
    print(f"Extracted frames: {start_frame} to {end_frame} ({num_frames} frames)")


if __name__ == "__main__":
    # example
    extract_rgb_frames(
        input_path="D:\\Oversea\\usc\\CSCI576_Multimedia\\Project\\DATA\\rgbs\\car.rgb",
        output_path="D:\\Oversea\\usc\\CSCI576_Multimedia\\Project\\DATA\\rgbs\\cut.rgb",
        width=960,
        height=540,
        start_frame=100,
        end_frame=150,
    )