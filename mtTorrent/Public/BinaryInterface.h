#pragma once

#include "Status.h"
#include "Alerts.h"
#include "ModuleString.h"
#include "ModuleArray.h"

namespace mtBI
{
	enum class MessageId
	{
		Init,
		Deinit,
		AddFromFile,	//char*, uint8_t[20]
		AddFromMetadata, //char*, uint8_t[20]
		Start,	//uint8_t[20], null
		Stop,	//uint8_t[20], null
		Remove,	//RemoveTorrentRequest, null
		GetTorrents,	//null,TorrentsList
		GetTorrentInfo,			//uint8_t[20], TorrentInfo
		GetTorrentStateInfo,	//uint8_t[20], TorrentStateInfo
		GetPeersInfo,	//uint8_t[20], TorrentPeersInfo
		GetSourcesInfo,	//uint8_t[20], SourcesInfo
		GetMagnetLinkProgress,	//uint8_t[20],MagnetLinkProgress
		GetMagnetLinkProgressLogs,	//MagnetLinkProgressLogsRequest,MagnetLinkProgressLogsResponse
		GetSettings, //null, SettingsInfo
		SetSettings, //SettingsInfo, null
		RefreshSource, //SourceId, null
		SetTorrentFilesSelection, //TorrentFilesSelectionRequest, null
		SetTorrentPath, //TorrentSetPathRequest, null
		AddPeer,	//AddPeerRequest, null
		GetPiecesInfo, //uint8_t[20], PiecesInfo
		GetUpnpInfo,	//null, string
		RegisterAlerts,	//RegisterAlertsRequest, null
		PopAlerts,		//null, AlertsList
		CheckFiles,		//hash, null
	};

	struct SourceId
	{
		uint8_t hash[20];
		mtt::string name;
	};

	struct SettingsInfo
	{
		uint32_t udpPort;
		uint32_t tcpPort;
		bool dhtEnabled;
		mtt::string directory;
		uint32_t maxConnections;
		bool upnpEnabled;
	};

	struct PiecesInfo
	{
		uint32_t piecesCount;
		uint32_t receivedCount;
		mtt::array<uint8_t> bitfield;
		mtt::array<uint32_t> requests;
	};

	struct MagnetLinkProgress
	{
		float progress;
		bool finished;
	};

	struct MagnetLinkProgressLogsRequest
	{
		uint32_t start;
		uint8_t hash[20];
	};

	struct MagnetLinkProgressLogsResponse
	{
		uint32_t fullcount;
		mtt::array<mtt::string> logs;
	};

	struct RemoveTorrentRequest
	{
		uint8_t hash[20];
		bool deleteFiles;
	};

	struct AddPeerRequest
	{
		uint8_t hash[20];
		mtt::string addr;
	};

	struct FileSelectionRequest
	{
		bool selected;
	};

	struct TorrentFilesSelectionRequest
	{
		uint8_t hash[20];
		mtt::array<FileSelectionRequest> selection;
	};

	struct TorrentsList
	{
		struct TorrentBasicInfo
		{
			uint8_t hash[20];
			bool active;
		};
		mtt::array<TorrentBasicInfo> list;
	};

	struct TorrentFile
	{
		mtt::string name;
		size_t size;
		bool selected;
	};

	struct TorrentInfo
	{
		mtt::array<TorrentFile> files;
		size_t fullsize;
		mtt::string name;
		mtt::string downloadLocation;
		mtt::string createdBy;
		size_t creationDate;
	};

	struct TorrentStateInfo
	{
		mtt::string name;
		float progress;
		float selectionProgress;
		size_t downloaded;
		size_t downloadSpeed;
		size_t uploaded;
		size_t uploadSpeed;
		uint32_t foundPeers;
		uint32_t connectedPeers;
		bool started;
		bool utmActive;
		bool checking;
		float checkingProgress;
		mtt::Status activeStatus;
	};

	struct PeerInfo
	{
		float progress;
		size_t dlSpeed;
		size_t upSpeed;
		mtt::string addr;
		mtt::string client;
		mtt::string country;
	};

	struct TorrentPeersInfo
	{
		mtt::array<PeerInfo> peers;
	};

	struct SourceInfo
	{
		mtt::string name;
		uint32_t peers;
		uint32_t seeds;
		uint32_t leechers;
		uint32_t nextCheck;
		uint32_t interval;
		enum : char { Stopped, Ready, Offline, Connecting, Announcing, Announced } status;
	};

	struct SourcesInfo
	{
		mtt::array<SourceInfo> sources;
	};

	struct TorrentSetPathRequest
	{
		uint8_t hash[20];
		mtt::string path;
	};

	struct RegisterAlertsRequest
	{
		uint32_t categoryMask;
	};

	struct Alert
	{
		mtt::AlertId id;
		uint8_t hash[20];
	};

	struct AlertsList
	{
		mtt::array<Alert> alerts;
	};
};
