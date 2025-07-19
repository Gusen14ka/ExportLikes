# ExportLikes

A Qt-based desktop application that allows you to export your liked tracks to Spotify and manage your Spotify library.

## Features ✨
- Import Liked Tracks from JSON files to your Spotify library
- Remove Last N Tracks from your Spotify liked songs
- OAuth 2.0 Authentication with Spotify API
- Cross-platform - works on Windows, Linux, and macOS
- Asynchronous Operations using Boost.Asio and coroutines
- Real-time Progress Tracking with progress bars and logging

## Prerequisites 🛠️

### Build Dependencies
- Qt 6 - Core, Widgets, Network modules
- Boost 1.75+ - Asio, Beast, System libraries
- CMake 3.14+ - Build system
- C++17 compatible compiler (GCC 8+, Clang 10+, MSVC 2019+)

### Runtime Dependencies
- Spotify Developer Account (for Client ID)
- OpenSSL 1.1.1+ (for HTTPS support)

## Building the Project 🏗️

### Linux/macOS
```bash
git clone https://github.com/yourusername/spotify-export-likes.git
cd spotify-export-likes
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
// For python script:
pip install pyinstaller
pyinstaller --onefile export_yandex_music_likes.py
```

### Windows
```powershell
git clone https://github.com/yourusername/spotify-export-likes.git
cd spotify-export-likes
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
// For python script:
pip install pyinstaller
pyinstaller --onefile export_yandex_music_likes.py
```

## Usage 🚀

1. **Import Liked Tracks**  
   - Launch the application  
   - Enter your Spotify Client ID and Redirect URI  
   - Select a JSON file containing your track data  
   - The application will authenticate with Spotify  
   - Tracks will be added to your Spotify library  

2. **Remove Last N Tracks**  
   - Launch the application  
   - Enter your Spotify Client ID and Redirect URI  
   - Specify the number of tracks to remove  
   - The application will remove the most recent tracks from your library  

### JSON Format
The application expects JSON files in the following format:
```json
[
  {"artist": "Artist Name", "title": "Track Title"},
  {"artist": "Another Artist", "title": "Another Track"}
]
```

## Configuration ⚙️

### Environment Variables
- `SPOTIFY_CLIENT_ID`: Your Spotify application Client ID  
- `SPOTIFY_REDIRECT_URI`: Your Spotify application Redirect URI (default: http://localhost:8888/callback)

### Command Line Options
```bash
spotify-export-likes [options]
```
Options:
- `--import <file>`       Import tracks from JSON file  
- `--remove <number>`     Remove last N tracks  
- `--client-id <id>`      Spotify Client ID  
- `--redirect-uri <uri>`  Spotify Redirect URI  

## Project Structure 📂

```
spotify-export-likes/
├── include/             # Header files
│   ├── SpotifyClient.hpp
│   ├── QtSpotifyClient.hpp
│   ├── AuthorizationServer.hpp
│   └── ...
├── src/                 # Source files
│   ├── SpotifyClient.cpp
│   ├── QtSpotifyClient.cpp
│   ├── AuthorizationServer.cpp
│   └── ...
├── forms/                  # UI files
│   └── exportlikes.ui
├── data/                  # config
│   └── cacert.pem
├── CMakeLists.txt       # Build configuration
├── README.md            # This file
└── .gitignore           # Git ignore rules
```

## Contributing 🤝
Contributions are welcome! Please follow these steps:
1. Fork the repository  
2. Create your feature branch (`git checkout -b feature/amazing-feature`)  
3. Commit your changes (`git commit -m 'Add some amazing feature'`)  
4. Push to the branch (`git push origin feature/amazing-feature`)  
5. Open a Pull Request  

## License 📄
This project is licensed under the MIT License - see the `LICENSE` file for details.

## Acknowledgments 🙏
- Spotify for their comprehensive API  
- Qt Company for the Qt framework  
- Boost developers for the excellent Asio and Beast libraries  
- OpenSSL team for secure communication  

*Note: This application is not affiliated with Spotify AB. Spotify is a registered trademark of Spotify AB.*
