# jellyfin-qbittorrent

Pause qbittorrent when there are two active jellyfin sessions and resume when there is less than two active sessions.

The program waits for a notification from jellyfin saying that a user session has started (notification sent by the jellyfin webhook plugin).

When it receives the notification, it starts monitoring active jellyfin sessions every two seconds with the `/Sessions` endpoint of the jellyfin API.

When the number of sessions is >= 2, it pauses qbittorrent with the `/api/v2/torrents/pause` endpoint of the qbittorrent API.

When the number of sessions is < 2, the torrents are resumed with the `/api/v2/torrents/resume` endpoint of the qbittorrent API.