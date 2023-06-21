# jellyfin-qbittorrent

Pause qbittorrent when jellyfin is playing something and resume when there is no active session left.

The program waits for a notification from jellyfin saying that a user session has started (notification sent by the jellyfin webhook plugin).

When it receives the notification, it pauses qbittorrent with the `/api/v2/torrents/pause` endpoint of the qbittorrent API.

It then starts getting active user sessions from jellyfin every 5 seconds with the `/Sessions` endpoint of the jellyfin API.

When the returned session array is empty, the torrents are resumed with the `/api/v2/torrents/resume` endpoint of the qbittorrent API.