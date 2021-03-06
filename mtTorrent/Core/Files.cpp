#include "Files.h"
#include "Configuration.h"

void mtt::Files::init(TorrentInfo& info)
{
	storage.init(info, mtt::config::getExternal().files.defaultDirectory);

	for (auto& f : info.files)
	{
		selection.files.push_back({ true, f });
	}

	progress.init(info.pieces.size());
}

void mtt::Files::addPiece(DownloadedPiece& piece)
{
	progress.addPiece(piece.index);
	storage.storePiece(piece);

	freshPieces.push_back(piece.index);
}

void mtt::Files::select(DownloadSelection& s)
{
	selection = s;
	progress.select(selection);
}

mtt::Status mtt::Files::prepareSelection()
{
	return storage.preallocateSelection(selection);
}
