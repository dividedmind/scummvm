/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 */

#include "common/util.h"
#include "backends/fs/abstract-fs.h"
#include "backends/fs/fs-factory.h"

FilesystemNode::FilesystemNode() {
	_realNode = 0;
	_refCount = 0;
}

FilesystemNode::FilesystemNode(AbstractFilesystemNode *realNode) {
	_realNode = realNode;
	_refCount = new int(1);
}

FilesystemNode::FilesystemNode(const FilesystemNode &node) {
	_realNode = node._realNode;
	_refCount = node._refCount;
	if (_refCount)
		++(*_refCount);
}

FilesystemNode::FilesystemNode(const Common::String &p) {
	FilesystemFactory *factory = FilesystemFactory::makeFSFactory();

	if (p.empty() || p == ".")
		_realNode = factory->makeCurrentDirectoryFileNode();
	else
		_realNode = factory->makeFileNodePath(p);
	_refCount = new int(1);
}

FilesystemNode::~FilesystemNode() {
	decRefCount();
}

FilesystemNode &FilesystemNode::operator= (const FilesystemNode &node) {
	if (node._refCount)
		++(*node._refCount);

	decRefCount();

	_realNode = node._realNode;
	_refCount = node._refCount;

	return *this;
}

bool FilesystemNode::operator<(const FilesystemNode& node) const {
	if (isDirectory() != node.isDirectory())
		return isDirectory();

	return scumm_stricmp(getDisplayName().c_str(), node.getDisplayName().c_str()) < 0;
}

void FilesystemNode::decRefCount() {
	if (_refCount) {
		assert(*_refCount > 0);
		--(*_refCount);
		if (*_refCount == 0) {
			delete _refCount;
			delete _realNode;
		}
	}
}

bool FilesystemNode::exists() const {
	if (_realNode == 0)
		return false;

	return _realNode->exists();
}

FilesystemNode FilesystemNode::getChild(const Common::String &n) const {
	if (_realNode == 0)
		return *this;

	assert(_realNode->isDirectory());
	AbstractFilesystemNode *node = _realNode->getChild(n);
	return FilesystemNode(node);
}

bool FilesystemNode::getChildren(FSList &fslist, ListMode mode, bool hidden) const {
	if (!_realNode || !_realNode->isDirectory())
		return false;

	AbstractFSList tmp;

	if (!_realNode->getChildren(tmp, mode, hidden))
		return false;

	fslist.clear();
	for (AbstractFSList::iterator i = tmp.begin(); i != tmp.end(); ++i) {
		fslist.push_back(FilesystemNode(*i));
	}

	return true;
}

Common::String FilesystemNode::getDisplayName() const {
	assert(_realNode);
	return _realNode->getDisplayName();
}

Common::String FilesystemNode::getName() const {
	assert(_realNode);
	return _realNode->getName();
}

FilesystemNode FilesystemNode::getParent() const {
	if (_realNode == 0)
		return *this;

	AbstractFilesystemNode *node = _realNode->getParent();
	if (node == 0) {
		return *this;
	} else {
		return FilesystemNode(node);
	}
}

Common::String FilesystemNode::getPath() const {
	assert(_realNode);
	return _realNode->getPath();
}

bool FilesystemNode::isDirectory() const {
	if (_realNode == 0)
		return false;

	return _realNode->isDirectory();
}

bool FilesystemNode::isReadable() const {
	if (_realNode == 0)
		return false;

	return _realNode->isReadable();
}

bool FilesystemNode::isWritable() const {
	if (_realNode == 0)
		return false;

	return _realNode->isWritable();
}

bool FilesystemNode::lookupFile(FSList &results, const Common::String &p, bool hidden, bool exhaustive, int depth) const {
	if (!isDirectory())
		return false;

	FSList children;
	FSList subdirs;
	Common::String pattern = p;

	pattern.toUppercase();

	// First match all files on this level
	getChildren(children, FilesystemNode::kListAll, hidden);
	for (FSList::iterator entry = children.begin(); entry != children.end(); ++entry) {
		if (entry->isDirectory()) {
			if (depth != 0)
				subdirs.push_back(*entry);
		} else {
			Common::String filename = entry->getName();
			filename.toUppercase();
			if (Common::matchString(filename.c_str(), pattern.c_str())) {
				results.push_back(*entry);

				if (!exhaustive)
					return true;	// Abort on first match if no exhaustive search was requested
			}
		}
	}

	// Now scan all subdirs
	for (FSList::iterator child = subdirs.begin(); child != subdirs.end(); ++child) {
		child->lookupFile(results, pattern, hidden, exhaustive, depth - 1);
		if (!exhaustive && !results.empty())
			return true;	// Abort on first match if no exhaustive search was requested
	}

	return !results.empty();
}
