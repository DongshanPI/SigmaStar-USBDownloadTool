
#include "stdafx.h"
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <sstream> 
#include <string>
#include "burnImage.h"
#include "md5.h"
#include "usbscsicmd.h"

#define LOAD_ADDR_UNSPECIFIED	0xFFFFFFFF
#define MAX_RUNCMD_LENGTH		(1024-1)	// should be identical with u-boot

struct loadinfo {
	unsigned int		addr;
	unsigned int		size;
	unsigned char		md5[MD5_LENGTH];
};

static unsigned char *_readfile(const char *name, int *size, int padding)
{
	FILE *fp = NULL;
	int fsize;
	unsigned char *buf = NULL;
	struct stat statbuf;
	size_t result;

	/* open file */
	if (fopen_s(&fp, name, "rb"))
	{
		log("_readfile: open %s fail\n", name);
		return NULL;
	}
	/* get file size */
	stat(name, &statbuf);
	fsize = statbuf.st_size;
	/* padding size */
	*size = (fsize < padding) ? padding : fsize;
	/* allocate buffer for reading file */
	buf = (unsigned char *)malloc(*size);
	if (!buf)
	{
		log("_readfile: malloc fail\n");
		goto EXIT;
	}
	else if (fsize < padding) {
		memset(&buf[fsize], 0, padding - fsize);
	}
	/* read file into buffer */
	result = fread(buf, 1, fsize, fp);
	if (result != fsize)
	{
		log("_readfile: %d to read, but only read %d\n", fsize, result);
	}
EXIT:
	if (fp)
		fclose(fp);
	return buf;
}

static void _md5sum(unsigned char *in, int in_size, unsigned char *output)
{
	MD5_CTX md5;

	MD5Init(&md5);
	MD5Update(&md5, in, in_size);
	MD5Final(&md5, output);
}

static int scsi_download_file(const char *filename, unsigned int load_addr)
{
	int len, retry = 5;
	unsigned char *buf = NULL;
	unsigned char result[4];
	struct loadinfo info;

	buf = _readfile(filename, &len, 0);
	if (!buf)
	{
		log("read file fail: %s\n", filename);
		return -1;
	}
	// send loadinfo to device
	memset(&info, 0, sizeof(info));
	info.addr = load_addr;
	info.size = len;
	_md5sum(buf, len, info.md5);
	while (retry--)
	{
		if (USB_ScsiLoadInfo(&info, sizeof(info)))
		{
			log("send loadinfo fail: %s\n", filename);
			continue;
		}
		// load file to device
		if (USB_ScsiWrite(buf, len, DATA_TRANSFER_LENGTH) != 0) {
			log("load fail: %s\n", filename);
			continue;
		}
		// wait device ready for md5sum verification
		Sleep(len/20480);
		// read back result
		memset(result, 0xff, sizeof(result));
		if (USB_ScsiRead(result, 4, 0) == 0)
		{
			if ((result[0] != 0x0D) || (result[1] != 0x00))
			{
				log("load fail: %s, result:%02x %02x\n", filename, result[0], result[1]);
				continue;
			}
			else {
				break;
			}
		}
		else {
			continue;
		}
	}
	free(buf);
	return 0;
}

int loadUpdater(const char *filename)
{
	int ret;
	int len;
	unsigned char *buf = NULL;

	buf = _readfile(filename, &len, 0);
	if (!buf)
	{
		return -1;
	}
	// load updater to device
	if (USB_ScsiWrite(buf, len, 1024) != 0) {
		log("download fail: %s\n", filename);
		ret = -1;
	}
	free(buf);
	return 0;
}

int loadUboot(const char *filename)
{
	return scsi_download_file(filename, LOAD_ADDR_UNSPECIFIED); // don't care load_addr, it is specified by updater
}

//==============================================================================================
//==============================================================================================

std::string trim(const std::string& str, const std::string& whitespace = " \t")
{
	const auto begin = str.find_first_not_of(whitespace);
	if (begin == std::string::npos)
		return ""; // no content

	const auto end = str.find_last_not_of(whitespace);
	const auto range = end - begin + 1;

	return str.substr(begin, range);
}

std::string reduce(const std::string& str, const std::string& fill = " ", const std::string& whitespace = " \t")
{
	// trim first
	auto result = trim(str, whitespace);

	// replace sub ranges
	auto beginSpace = result.find_first_of(whitespace);
	while (beginSpace != std::string::npos)
	{
		const auto endSpace = result.find_first_not_of(whitespace, beginSpace);
		const auto range = endSpace - beginSpace;

		result.replace(beginSpace, range, fill);

		const auto newStart = beginSpace + fill.length();
		beginSpace = result.find_first_of(whitespace, newStart);
	}

	return result;
}

int ParseScript(const char *filename)
{
	std::ifstream s(filename);

	if (s.is_open())
	{
		std::string line;
		while (getline(s, line)) {
			line = reduce(line);
			if (line[0] == '#' || line[0] == '%' || line.empty())
				continue;

			std::stringstream stream(line);
			std::string cmd;
			stream >> cmd;

			log("%s\n", line.c_str());
			if (cmd.compare("estar") == 0) {
				// keep parsing script
				std::string subscript;
				stream >> subscript;
				if (ParseScript(subscript.c_str()) != 0) {
					log("parse script fail: %s\n", subscript.c_str());
					return -1;
				}
			}
			else if (cmd.compare("tftp") == 0) {
				std::string addr, image;
				stream >> addr >> image;
				// send image with the specified load address & filename
				if (scsi_download_file(image.c_str(), std::stoul(addr, nullptr, 16)) != 0) {
					log("load file fail: %s\n", image.c_str());
					return -1;
				}
			}
			else {
				unsigned char result[4];
				int retry = 0;
				// cmd to run by u-boot
				//fprintf(stderr, "cmd: %s\n", line.c_str());
				if (line.length() > MAX_RUNCMD_LENGTH) {
					log("command is too long to support. cmd: %s\n", line.c_str());
					return -1;
				}
				while (retry++ < 5)
				{
					USB_ScsiRunCmd(line.c_str(), line.length());
					// read back result
					memset(result, 0xff, sizeof(result));
					if (USB_ScsiRead(result, 4, 0) == 0)
					{
						if ((result[0] == 0x0D) && (result[1] == 0x00))
							break;
						log("%s [retry %d]\n", line.c_str(), retry);
					}
				}

				if (retry >= 5)
					return -1;
			}
		}
	}
	else {
		log("open script fail: %s\n", filename);
		return -1;
	}
	return 0;
}

int RunScript(const char *filename)
{
	return ParseScript(filename);
}

int IdentifyDevice(CString str) {
	CString  vid, pid;
	int pos_vid, pos_pid;
	pos_vid = str.Find(_T("vid"));
	pos_pid = str.Find(_T("pid"));
	vid = str.Mid(pos_vid + 4, 4);
	pid = str.Mid(pos_pid + 4, 4);
	if (vid == "114d" && pid == "23fb")
		return 1;
	else if (vid == "1d6b" && pid == "0102")
		return 1;
	else return 0;
}//modified by TX