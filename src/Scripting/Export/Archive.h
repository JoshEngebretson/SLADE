
//namespace Lua
//{

vector<Archive*> allArchives(bool resources_only)
{
	vector<Archive*> list;
	for (unsigned a = 0; a < App::archiveManager().numArchives(); a++)
	{
		Archive* archive = App::archiveManager().getArchive(a);

		if (resources_only && !App::archiveManager().archiveIsResource(archive))
			continue;

		list.push_back(archive);
	}
	return list;
}

string formattedEntryName(ArchiveEntry& self, bool include_path, bool include_extension, bool name_uppercase)
{
	string name;
	if (include_path)
		name = self.getPath();
	if (name_uppercase)
		name += include_extension ? self.getUpperName() : self.getUpperNameNoExt();
	else
		name += self.getName(!include_extension);
	return name;
}

vector<ArchiveTreeNode*> archiveDirSubDirs(ArchiveTreeNode& self)
{
	vector<ArchiveTreeNode*> dirs;
	for (auto child : self.allChildren())
		dirs.push_back((ArchiveTreeNode*)child);
	return dirs;
}

void registerArchive(sol::state& lua)
{
	lua.new_usertype<Archive>(
		"Archive",

		// No constructor
		"new", sol::no_constructor,

		// Properties
		"filename",	sol::property([](Archive& self) { return self.filename(); }),
		"entries",	sol::property(&Archive::luaAllEntries),
		"rootDir",	sol::property(&Archive::rootDir),

		// Functions
		"filenameNoPath",			[](Archive& self) { return self.filename(false); },
		"entryAtPath",				&Archive::entryAtPath,
		"dirAtPath",				&Archive::luaGetDir,
		"createEntry",				&Archive::luaCreateEntry,
		"createEntryInNamespace",	&Archive::luaCreateEntryInNamespace,
		"removeEntry",				&Archive::removeEntry,
		"renameEntry",				&Archive::renameEntry
	);

	// Register all subclasses
	// (perhaps it'd be a good idea to make Archive not abstract and handle
	//  the format-specific stuff somewhere else, rather than in subclasses)
	#define REGISTER_ARCHIVE(type) lua.new_usertype<type>(#type, sol::base_classes, sol::bases<Archive>())
	REGISTER_ARCHIVE(WadArchive);
	REGISTER_ARCHIVE(WadArchive);
	REGISTER_ARCHIVE(ZipArchive);
	REGISTER_ARCHIVE(LibArchive);
	REGISTER_ARCHIVE(DatArchive);
	REGISTER_ARCHIVE(ResArchive);
	REGISTER_ARCHIVE(PakArchive);
	REGISTER_ARCHIVE(BSPArchive);
	REGISTER_ARCHIVE(GrpArchive);
	REGISTER_ARCHIVE(RffArchive);
	REGISTER_ARCHIVE(GobArchive);
	REGISTER_ARCHIVE(LfdArchive);
	REGISTER_ARCHIVE(HogArchive);
	REGISTER_ARCHIVE(ADatArchive);
	REGISTER_ARCHIVE(Wad2Archive);
	REGISTER_ARCHIVE(WadJArchive);
	REGISTER_ARCHIVE(WolfArchive);
	REGISTER_ARCHIVE(GZipArchive);
	REGISTER_ARCHIVE(BZip2Archive);
	REGISTER_ARCHIVE(TarArchive);
	REGISTER_ARCHIVE(DiskArchive);
	REGISTER_ARCHIVE(PodArchive);
	REGISTER_ARCHIVE(ChasmBinArchive);
	#undef REGISTER_ARCHIVE
}

void registerArchiveEntry(sol::state& lua)
{
	lua.new_usertype<ArchiveEntry>(
		"ArchiveEntry",

		// No constructor
		"new", sol::no_constructor,

		// Properties
		"name",		sol::property([](ArchiveEntry& self) { return self.getName(); }),
		"path",		sol::property([](ArchiveEntry& self) { return self.getPath(); }),
		"type",		sol::property(&ArchiveEntry::getType),
		"size",		sol::property(&ArchiveEntry::getSize),
		"index",	sol::property([](ArchiveEntry& self) { return self.getParentDir()->entryIndex(&self); }),

		// Functions
		"formattedName", sol::overload(
			[](ArchiveEntry& self)
			{ return formattedEntryName(self, true, true, false); },
			[](ArchiveEntry& self, bool include_path)
			{ return formattedEntryName(self, include_path, true, false); },
			[](ArchiveEntry& self, bool include_path, bool include_extension)
			{ return formattedEntryName(self, include_path, include_extension, false); },
			&formattedEntryName
		),
		"formattedSize",	&ArchiveEntry::getSizeString,
		"crc32",			[](ArchiveEntry& self) { return Misc::crc(self.getData(), self.getSize()); }
	);
}

void registerArchiveTreeNode(sol::state& lua)
{
	lua.new_usertype<ArchiveTreeNode>(
		"ArchiveDir",

		// No constructor
		"new", sol::no_constructor,

		// Properties
		"name",				sol::property(&ArchiveTreeNode::getName),
		"archive",			sol::property(&ArchiveTreeNode::archive),
		"entries",			sol::property(&ArchiveTreeNode::luaGetEntries),
		"parent",			sol::property([](ArchiveTreeNode& self) { return (ArchiveTreeNode*)self.getParent(); }),
		"path",				sol::property(&ArchiveTreeNode::getPath),
		"subDirectories",	sol::property(&archiveDirSubDirs)
	);
}

void registerEntryType(sol::state& lua)
{
	lua.new_usertype<EntryType>(
		"EntryType",

		// No constructor
		"new", sol::no_constructor,

		// Properties
		"id",			sol::property(&EntryType::getId),
		"name",			sol::property(&EntryType::getName),
		"extension",	sol::property(&EntryType::getExtension),
		"formatId",		sol::property(&EntryType::getFormat),
		"editor",		sol::property(&EntryType::getEditor),
		"category",		sol::property(&EntryType::getCategory)
	);
}
	
void registerArchivesNamespace(sol::state& lua)
{
	sol::table archives = lua.create_table("archives");

	archives.set_function("all", sol::overload(
		&allArchives,
		[]() { return allArchives(false); }
	));

	archives.set_function("openFile", [](const char* filename)
	{
		return App::archiveManager().openArchive(filename);
	});

	archives.set_function("close", sol::overload(
		[](Archive* archive) { return App::archiveManager().closeArchive(archive); },
		[](int index) { return App::archiveManager().closeArchive(index); }
	));

	archives.set_function("closeAll", []()
	{
		App::archiveManager().closeAll();
	});

	archives.set_function("fileExtensionsString", []()
	{
		return App::archiveManager().getArchiveExtensionsString();
	});

	archives.set_function("baseResource", []()
	{
		return App::archiveManager().baseResourceArchive();
	});

	archives.set_function("baseResourcePaths", []()
	{
		return App::archiveManager().baseResourcePaths();
	});

	archives.set_function("openBaseResource", [](int index)
	{
		return App::archiveManager().openBaseResource(index - 1);
	});

	archives.set_function("programResource", []()
	{
		return App::archiveManager().programResourceArchive();
	});

	archives.set_function("recentFiles", []()
	{
		return App::archiveManager().recentFiles();
	});

	archives.set_function("bookmarks", []()
	{
		return App::archiveManager().bookmarks();
	});

	archives.set_function("addBookmark", [](ArchiveEntry* entry)
	{
		App::archiveManager().addBookmark(entry);
	});

	archives.set_function("removeBookmark", [](ArchiveEntry* entry)
	{
		App::archiveManager().deleteBookmark(entry);
	});
}

void registerArchiveTypes(sol::state& lua)
{
	registerArchive(lua);
	registerArchiveEntry(lua);
	registerEntryType(lua);
	registerArchiveTreeNode(lua);
}

//} // namespace Lua