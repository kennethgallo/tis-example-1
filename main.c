from tis_camera import TIS, SinkFormats
import cv2
from datetime import datetime
from pathlib import Path


def save_capture_with_settings(cam, img, frame_num, session_dir):
    source = cam.get_source()

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    base_name = f"capture_{frame_num}_{timestamp}"

    image_path = session_dir / f"{base_name}.png"
    settings_path = session_dir / f"{base_name}.json"

    # TIS helper uses BGRx, so drop the last channel for OpenCV saving
    if len(img.shape) == 3 and img.shape[2] == 4:
        save_img = img[:, :, :3]
    else:
        save_img = img

    image_saved = cv2.imwrite(str(image_path), save_img)

    if not image_saved:
        print(f"Failed to save image: {image_path}")
        return

    try:
        state = source.get_property("tcam-properties-json")

        with open(settings_path, "w", encoding="utf-8") as f:
            f.write(state)

    except Exception as e:
        print(f"Could not save camera JSON settings: {e}")
        return

    print(f"Saved {image_path}")
    print(f"Saved {settings_path}")


def main():
    cam = TIS()

    session_timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    session_dir = Path(f"capture_session_{session_timestamp}")
    session_dir.mkdir(parents=True, exist_ok=True)

    cam.open_device(
        serial=None,
        width=640,
        height=480,
        framerate="30/1",
        sinkformat=SinkFormats.BGRA,
        showvideo=True
    )

    try:
        cam.set_property("TriggerMode", "On")
    except Exception as e:
        print(f"Could not enable trigger mode: {e}")

    if not cam.start_pipeline():
        print("Could not start pipeline.")
        return

    frame_num = 0

    print("Live preview started.")
    print(f"Session folder: {session_dir.resolve()}")
    print("Press Enter to trigger and save one image plus JSON settings.")
    print("Type q and press Enter to quit.")

    try:
        while True:
            cmd = input("> ").strip().lower()

            if cmd == "q":
                break

            try:
                cam.execute_command("TriggerSoftware")
            except Exception as e:
                print(f"Trigger failed: {e}")

            cam.snap_image(1)
            img = cam.get_image()

            if img is None:
                print("No image captured.")
                continue

            save_capture_with_settings(cam, img, frame_num, session_dir)
            frame_num += 1

    finally:
        try:
            cam.set_property("TriggerMode", "Off")
        except Exception:
            pass

        cam.stop_pipeline()
        print("Camera stopped.")


if __name__ == "__main__":
    main()