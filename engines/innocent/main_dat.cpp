#include "innocent/main_dat.h"

#include "common/endian.h"
#include "common/util.h"

using namespace Common;
using namespace std;

namespace Innocent {

MainDat::MainDat(Resources *res) : Datafile(res), _data(0) {}

MainDat::~MainDat() {
	if (_data)
		delete[] _data;
}

void MainDat::readFile(SeekableReadStream &stream) {
	_dataLen = stream.readUint16LE();
	debug(kDataRead, "length of main data is %d", _dataLen);

	_data = new byte[_dataLen];
	stream.seek(0);
	stream.read(_data, _dataLen);
	descramble();

	stream.read(_footer, kFooterLen);
}

void MainDat::descramble() {
	for (int i = 2; i < _dataLen; i++)
		_data[i] ^= 0x6f;
}

uint16 MainDat::imagesCount() const {
	return READ_LE_UINT16(_footer + kImagesCount);
}

list<MainDat::GraphicFile> MainDat::graphicFiles() const {
	uint16 file_count = READ_LE_UINT16(_footer + kGraphicFileCount);
	uint16 names_offset = READ_LE_UINT16(_footer + kGraphicFileNames);

	byte *data = _data + names_offset;
	list<GraphicFile> files;
	for (; file_count > 0; file_count--) {
		GraphicFile file;
		file.data_set = READ_LE_UINT16(data);
		data += 2;
		file.filename = data;
		files.push_back(file);
		while (*data)
			data++;
		while (!*data)
			data++;
	}

	return files;
}

} // End of namespace Innocent
