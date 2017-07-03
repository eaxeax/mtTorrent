#include "DhtCommunication.h"
#include "Network.h"
#include "TorrentDefines.h"
#include "BencodeParser.h"
#include "PacketHelper.h"
#include "utils/Base32.h"

struct NodeId
{
	uint8_t data[20];

	NodeId()
	{
	}

	NodeId(char* buffer)
	{
		copy(buffer);
	}

	void copy(char* buffer)
	{
		for (int i = 0; i < 20; i++)
			data[i] = (uint8_t)buffer[i];
	}

	bool shorterThan(NodeId& r)
	{
		for (int i = 0; i < 20; i++)
		{
			if (data[i] < r.data[i])
				return true;
		}

		return false;
	}
};

struct NodeInfo
{
	NodeId id;

	std::string addr;
	std::vector<uint8_t> addrBytes;
	uint16_t port;

	void parse(char* buffer, bool v6)
	{
		id.copy(buffer);
		buffer += 20;

		size_t addrSize = v6 ? 16 : 4;
		addrBytes.assign(buffer, buffer + addrSize);

		if (!v6)
		{
			uint32_t ip = swap32(*reinterpret_cast<uint32_t*>(&addrBytes));

			uint8_t ipAddr[4];
			ipAddr[3] = *reinterpret_cast<uint8_t*>(&ip);
			ipAddr[2] = *(reinterpret_cast<uint8_t*>(&ip) + 1);
			ipAddr[1] = *(reinterpret_cast<uint8_t*>(&ip) + 2);
			ipAddr[0] = *(reinterpret_cast<uint8_t*>(&ip) + 3);

			addr = std::to_string(ipAddr[0]) + "." + std::to_string(ipAddr[1]) + "." + std::to_string(ipAddr[2]) + "." + std::to_string(ipAddr[3]);
		}
		else
		{
			addr.resize(41);
			auto ptr = &addr[0];

			for (size_t i = 0; i < 16; i+=2)
			{
				sprintf_s(ptr, 6, "%02X%02X:", addrBytes[i], addrBytes[i + 1]);
				ptr += 5;
			}

			addr.resize(39);
		}

		buffer += addrSize;

		port = _byteswap_ushort(*reinterpret_cast<const uint16_t*>(buffer));
	}
};

void mtt::DhtCommunication::test()
{
	const char* dhtRoot = "dht.transmissionbt.com";
	const char* dhtRootPort = "6881";

	bool ipv6 = true;

	boost::asio::io_service io_service;
	udp::resolver resolver(io_service);

	udp::socket sock(io_service);
	sock.open(ipv6 ? udp::v6() : udp::v4());

	std::string myId(20,0);
	for (int i = 0; i < 20; i++)
	{
		myId[i] = 5 + i * 5;
	}

	std::string targetIdBase32 = "ZEF3LK3MCLY5HQGTIUVAJBFMDNQW6U3J";
	auto targetId = base32decode(targetIdBase32);

	const char* clientId = "mt02";
	uint16_t transactionId = 54535;

	PacketBuilder packet(104);
	packet.add("d1:ad2:id20:",12);
	packet.add(myId.data(), 20);
	packet.add("9:info_hash20:",14);
	packet.add(targetId.data(), 20);
	packet.add("e1:q9:get_peers1:v4:",20);
	packet.add(clientId,4);
	packet.add("1:t2:",5);
	packet.add(reinterpret_cast<char*>(&transactionId),2);
	packet.add("1:y1:qe",7);

	std::vector<NodeInfo> receivedNodes;

	try
	{	
		auto message = sendUdpRequest(sock, resolver, packet.getBuffer(), dhtRoot, dhtRootPort, 5000, ipv6);

		if (!message.empty())
		{
			BencodeParser parser;
			parser.parse(message);

			if (parser.parsedData.isMap())
			{
				auto obj = parser.parsedData.dic->find("r");

				if (obj != parser.parsedData.dic->end() && obj->second.isMap())
				{
					auto nodesV4 = obj->second.dic->find("nodes");
					auto nodesV6 = obj->second.dic->find("nodes6");

					bool v6nodes = (nodesV6 != obj->second.dic->end());
					auto nodes = v6nodes ? nodesV6 : nodesV4;

					if (nodes != obj->second.dic->end() && nodes->second.type == mtt::BencodeParser::Object::Text)
					{
						auto& data = nodes->second.txt;

						for (size_t pos = 0; pos < data.size(); pos += 38)
						{
							NodeInfo info;
							info.parse(&data[pos], v6nodes);

							receivedNodes.push_back(info);
						}	
					}
				}
			}

		}
	}
	catch (const std::exception&e)
	{
		DHT_LOG("DHT exception: " << e.what() << "\n");
	}
}
