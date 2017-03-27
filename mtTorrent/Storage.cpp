#include "Storage.h"
#include <fstream>
#include <boost/filesystem.hpp>

mtt::Storage::Storage(size_t piecesz)
{
	pieceSize = piecesz;
}

void mtt::Storage::setPath(std::string p)
{
	path = p;
}

void mtt::Storage::storePiece(DownloadedPiece& piece)
{
	for (auto& s : selection.files)
	{
		auto& f = s.file;

		if (f.startPieceIndex <= piece.index && f.endPieceIndex >= piece.index)
		{
			storePiece(f, piece);
		}
	}
}

void mtt::Storage::flush()
{
	for (auto& s : selection.files)
	{
		flush(s.file);
	}
}

void mtt::Storage::selectFiles(std::vector<mtt::File>& files)
{
	selection.files.clear();

	for (auto& f : files)
	{
		selection.files.push_back({ f });
		preallocate(f);
	}
}

void mtt::Storage::saveProgress()
{

}

void mtt::Storage::loadProgress()
{

}

void mtt::Storage::storePiece(File& file, DownloadedPiece& piece)
{
	/*auto& outBuffer = filesBuffer[file.id];

	size_t bufferStartOffset = (piece.index - file.startPieceIndex)*normalPieceSize;

	memcpy(&outBuffer[0] + bufferStartOffset, piece.data.data(), normalPieceSize);
	*/
	unsavedPieces[file.id].push_back(piece);

	if (unsavedPieces[file.id].size() > piecesCacheSize)
		flush(file);
}

void mtt::Storage::flush(File& file)
{
	auto path = getFullpath(file);
	std::string dirPath = path.substr(0, path.find_last_of('\\'));
	boost::filesystem::path dir(dirPath);
	if (!boost::filesystem::exists(dir))
	{
		if (!boost::filesystem::create_directory(dir))
			return;
	}

	auto& pieces = unsavedPieces[file.id];

	std::ofstream fileOut(path, std::ios_base::binary | std::ios_base::in);

	for (auto& p : pieces)
	{	
		auto piecePos = file.startPiecePos + (p.index - file.startPieceIndex)*pieceSize;
		fileOut.seekp(piecePos);
		fileOut.write(p.data.data(), p.dataSize);
	}

	unsavedPieces[file.id].clear();
}

std::vector<mtt::PieceInfo> mtt::Storage::checkFileHash(File& file)
{
	std::vector<mtt::PieceInfo> out;

	auto fullpath = getFullpath(file);
	boost::filesystem::path dir(fullpath);
	if (boost::filesystem::exists(dir))
	{
		std::ifstream fileIn(fullpath, std::ios_base::binary);
		std::vector<char> buffer(pieceSize);
		size_t idx = 0;
		
		if (file.startPiecePos)
		{
			auto startOffset = pieceSize - (file.startPiecePos % pieceSize);
			out.push_back(PieceInfo{});
			fileIn.read(buffer.data(), startOffset);
		}	

		PieceInfo info;
		out.reserve(file.size % pieceSize);

		while (fileIn)
		{
			fileIn.read(buffer.data(), pieceSize);
			SHA1((unsigned char*)buffer.data(), fileIn.gcount(), (unsigned char*)&info.hash);
			out.push_back(info);
		}
	}

	return out;
}

void mtt::Storage::preallocate(File& file)
{
	auto fullpath = getFullpath(file);
	boost::filesystem::path dir(fullpath);
	if (!boost::filesystem::exists(dir) || boost::filesystem::file_size(dir) != file.size)
	{
		std::ofstream fileOut(fullpath, std::ios_base::binary);
		fileOut.seekp(file.size - 1);
		fileOut.put(0);
	}
}

std::string mtt::Storage::getFullpath(File& file)
{
	std::string filePath;

	for (auto& p : file.path)
	{
		if (!filePath.empty())
			filePath += "\\";

		filePath += p;
	}

	return path + filePath;
}
