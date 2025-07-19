import json
import os
import asyncio
import sys
from dotenv import load_dotenv
from yandex_music import ClientAsync
from pathlib import Path
import tkinter as tk
from tkinter import filedialog

def get_env_path():
    if sys.platform.startswith("win"):
        # LOCALAPPDATA → C:\Users\<User>\AppData\Local
        base = Path(os.getenv("LOCALAPPDATA",
                              Path.home() / "AppData" / "Local"))
    elif sys.platform == "darwin":
        base = Path.home() / "Library" / "Application Support"
    else:  # linux/unix
        base = Path.home() / ".config"
    return base / "ExportLikes" / ".env"

def save_tracks_to_json(tracks):
    # Спрячем главное окно Tk
    root = tk.Tk()
    root.withdraw()

    # Открываем диалог «Сохранить как»
    file_path = filedialog.asksaveasfilename(
        title="Save Spotify tracks as JSON",
        defaultextension=".json",
        filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
    )

    # Если пользователь нажал Отмена, file_path == ''
    if not file_path:
        print("Save cancelled.")
        return False

    # Записываем список tracks (например, список словарей) в JSON
    try:
        with open(file_path, "w", encoding="utf-8") as f:
            json.dump(tracks, f, ensure_ascii=False, indent=2)
        print(f"Tracks saved to {file_path}")
        return True
    except Exception as e:
        print(f"Error saving file: {e}")
        return False

# Helper: split sequence into chunks len = n
def chunker(seq, n):
    for i in range(0, len(seq), n):
        yield seq[i: i + n]
async def main():
    # Load environment variables (.env must contain YANDEX_TOKEN)
    env_path = get_env_path()
    load_dotenv(dotenv_path=env_path)
    token = os.getenv("YANDEX_TOKEN")
    if not token:
        print("Error: YANDEX_TOKEN not set")
        print("ENV path: " + str(env_path))
        return

    # Initialize client and fetch account status internally
    client = await ClientAsync(token).init()

    # Get liked track (in short objects)
    likes = await client.users_likes_tracks()
    print(f"Total liked tracks: {len(likes)}")

    # Split into chunks
    # For each chunk fetch full Track object and get artist and title
    data = []
    chunk_size = 50
    for idx, chunk in enumerate(chunker(likes, chunk_size), start=1):
        print(f"Fetching {idx} chunk...")

        # Schedule fetches in current chunk
        task = [short.fetch_track_async() for short in chunk]
        tracks_in_chunk = await asyncio.gather(*task)

        # Add tracks in chunk to data
        for tr in tracks_in_chunk:
            print(f"Work with track {tr.title}")
            data.append({
                "artist": tr.artists[0].name if tr.artists else "",
                "title": tr.title
            })
    data.reverse()
    # Save to json file
    out_path = "yandex_likes.json"
    save_tracks_to_json(data)

    print(f"Parsing likes from Yandex Music has been completed"
          f"Saved {len(data)} track into {out_path}")

if __name__ == "__main__":
    asyncio.run(main())
