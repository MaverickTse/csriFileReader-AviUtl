
#include <string>
#include <memory>
#include <list>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <atlstr.h>
#include "input.h"
#include "csri.h"

typedef struct {
	bool isSRT;
	csri_fmt* csri_format; //a struct, new/del
	csri_rend* renderer; //a pointer, free with csri
	csri_inst* track; //free with csri
	BITMAPINFOHEADER* bmi; //a struct, new/del
	int width;
	int height;
	LONGLONG duration;
	void* style;
	
}SubFileHandle;

INPUT_PLUGIN_TABLE input_plugin_table = {
	INPUT_PLUGIN_FLAG_VIDEO,
	"CSRI Subtitle Reader",
	"SubStationAlpha(*.ssa)\0*.ssa\0"
	"AdvancedSubStation(*.ass)\0*.ass\0"
	"SubRip(*.srt)\0*.srt\0"
	//"VobSub(*.idx)\0*.idx\0"
	//"GraphicalFormat(*.sub)\0*.sub\0"
	//"PowerDivX(*.psb)\0*.psb\0"
	//"DVDMaestro(*.son)\0*.son\0"
	//"Scenarist(*.sst)\0*.sst\0"
	"UniversalSubtitleFormat(*.usf)\0*.usf\0"
	"SAMI(*.smi)\0*.smi\0",
	"CSRI Subtitle Reader 1.0 for AviUtl by Maverick Tse",
	func_init,
	func_exit,
	func_open,
	func_close,
	func_info_get,
	func_read_video,
	NULL, //func_read audio: NA
	NULL, //func_is_keyframe: NA
	NULL  //func_config: RESERVED
};

EXTERN_C INPUT_PLUGIN_TABLE __declspec(dllexport) * __stdcall GetInputPluginTable(void)
{
	return &input_plugin_table;
}

BOOL func_init(void)
{
	//TODO
	return TRUE;
}

BOOL func_exit(void)
{
	//TODO
	return TRUE;
}
INPUT_HANDLE func_open(LPSTR file)
{
	//TODO
	/* //DEBUG Use
	bool isFilenameUnicode = IsTextUnicode(file, sizeof(file), NULL);
	if (!isFilenameUnicode)
	{
		MessageBox(nullptr, "Filename is not in Unicode", "DEBUG INFO", MB_OK);
	}
	else
	{
		MessageBox(nullptr, "Filename is Unicode", "DEBUG INFO", MB_OK);
	}*/
	
	SubFileHandle* fh = new SubFileHandle;
	SecureZeroMemory(fh, sizeof(SubFileHandle));
	std::string filename(file);
	std::transform(filename.begin(), filename.end(), filename.begin(), tolower); //to lower case
	fh->isSRT = false;
	size_t idx = std::string::npos;
	bool isSRT=false, isSSA=false, isAAS=false;
	idx = filename.find(".srt");
	if (idx != std::string::npos) { isSRT = true; fh->isSRT = true; }

	//Open and parse setting ini
	std::ifstream hINIfile("csri_reader.ini");
	std::string INI_content;
	std::stringstream INI_buffer;
	bool AutoSize = false;
	int defaultWidth = 1280;
	int defaultHeight = 720;
	long defaultSec = 10800;
	if (!hINIfile.bad())
	{
		INI_buffer << hINIfile.rdbuf();
		hINIfile.close();
		INI_content = INI_buffer.str();
		INI_buffer.clear();
		//regex parse

		//setup patterns
		std::regex pAutoSize("AutoSize\\s*=\\s*(\\d+)$", std::regex_constants::icase);
		std::regex piniWidth("Width\\s*=\\s*(\\d+)$", std::regex_constants::icase);
		std::regex piniHeight("Height\\s*=\\s*(\\d+)$", std::regex_constants::icase);
		std::regex piniSec("Duration_s\\s*=\\s*(\\d+)$", std::regex_constants::icase);
		//setup match storage
		std::smatch mAutoSize, miniWidth, miniHeight, miniSec;
		//search
		bool foundAS = std::regex_search(INI_content, mAutoSize, pAutoSize);
		bool foundiW = std::regex_search(INI_content, miniWidth, piniWidth);
		bool foundiH = std::regex_search(INI_content, miniHeight, piniHeight);
		bool foundSec = std::regex_search(INI_content, miniSec, piniHeight);
		//Parse
		if (foundAS)
		{
			std::string ASbuf = mAutoSize[1];
			AutoSize = (bool)std::stoi(ASbuf);
		}
		if (foundiW)
		{
			std::string iWbuf = miniWidth[1];
			defaultWidth = std::stoi(iWbuf);
		}
		if (foundiH)
		{
			std::string iHbuf = miniHeight[1];
			defaultHeight = std::stoi(iHbuf);
		}
		if (foundSec)
		{
			std::string iSecbuf = miniSec[1];
			defaultSec = std::stol(iSecbuf);
		}
	}
	//parse file to get length of subtitle, and original dimension
	//Use 1280x720 as default if not defined(SRT)
	std::ifstream content(file, std::ios::in);
	std::string content_str;
	std::stringstream content_buf;
	if (!content.bad())
	{
		content_buf << content.rdbuf();
		content.close();
		content_str = content_buf.str();
		content_buf.clear();
		
	}
	else
	{
		MessageBox(nullptr, "Failed to open file", "Subtitle ERROR", MB_OK);
		delete fh;
		return FALSE;
	}
	/*
	if (content.is_open())
	{
		TCHAR achar;
		while (content.get(achar)) //read all content into content_str
		{
			content_str.append(1, achar);
		}
		content.close();
	}*/

	if (AutoSize)
	{
		//search pattern for width and height
		std::regex pWidth("PlayResX:\\s*(\\d+)$", std::regex_constants::icase | std::regex_constants::ECMAScript);
		std::regex pHeight("PlayResY:\\s*(\\d+)$", std::regex_constants::icase | std::regex_constants::ECMAScript);
		std::smatch mWidth, mHeight; //for capture group
		//start search
		bool foundWidth = std::regex_search(content_str, mWidth, pWidth);
		bool foundHeight = std::regex_search(content_str, mHeight, pHeight);
		//store result
		if (foundWidth)
		{
			fh->width = std::stoi(mWidth[1]);
		}
		else
		{
			fh->width = 1280;
		}

		if (foundHeight)
		{
			fh->height = std::stoi(mHeight[1]);
		}
		else
		{
			fh->height = 720;
		}
	}
	else
	{
		fh->width = defaultWidth;
		fh->height = defaultHeight;
	}
	
	//Further check if it is old SSA or newer AAS
	if (!isSRT)
	{
		std::regex pASS("ScriptType:\\s+?v4\\.00\\+", std::regex_constants::icase);
		bool foundASS = std::regex_search(content_str, pASS);
		if (foundASS)
		{
			isAAS = true;
		}
		else
		{
			isSSA = true;
		}
	}

	//find subtitle length
	if (isSRT)
	{
		//Find max duration in SRT file
		LONGLONG timestamp = 0;
		std::regex ptime("(\\d{2}):(\\d{2}):(\\d{2}),(\\d{3})", std::regex_constants::icase); //hh:mm:ss,mss
		//loop through each timestamp and convert to millisecounds
		std::sregex_iterator sit_ini(content_str.cbegin(), content_str.cend(), ptime);
		std::sregex_iterator sit_fin;
		std::for_each(sit_ini, sit_fin, [&timestamp](const std::smatch& m){
			int hh, mm, ss, ms;
			hh = std::stoi(m[1]);
			mm = std::stoi(m[2]);
			ss = std::stoi(m[3]);
			ms = std::stoi(m[4]);
			LONGLONG totaltime = ms + ss * 1000 + mm * 60 * 1000 + hh * 60 * 60 * 1000;
			if (totaltime > timestamp)
			{
				timestamp = totaltime;
			}
		});
		if (timestamp)
		{
			fh->duration = timestamp;
		}
		else
		{
			fh->duration = defaultSec * 1000;
		}
		
	}
	else if (isAAS||isSSA)
	{
		//Find max duration in ASS/SSA file
		
		LONGLONG timestamp = 0;
		std::regex ptime("(\\d{1}):(\\d{2}):(\\d{2})[:.](\\d{2})", std::regex_constants::icase); //hh:mm:ss:ms
		//loop through each timestamp and convert to millisecounds
		std::sregex_iterator sit_ini(content_str.cbegin(), content_str.cend(), ptime);
		std::sregex_iterator sit_fin;
		std::for_each(sit_ini, sit_fin, [&timestamp](const std::smatch& m){
			int hh, mm, ss, ms10;
			hh = std::stoi(m[1]);
			mm = std::stoi(m[2]);
			ss = std::stoi(m[3]);
			ms10 = std::stoi(m[4]);
			LONGLONG totaltime = ms10 * 10 + ss * 1000 + mm * 60 * 1000 + hh * 60 * 60 * 1000;
			if (totaltime > timestamp)
			{
				timestamp = totaltime;
			}
		});
		if (timestamp)
		{
			fh->duration = timestamp;
		}
		else
		{
			fh->duration = defaultSec * 1000;
		}
		
	}
	else //other format
	{
		fh->duration = defaultSec * 1000; //assume 3hr
	}
	content_str.clear();
	
	/************CSRI routines*******************************/
	/*
	//List all renderer
	std::vector<std::string> renderer_names;
	std::vector<csri_rend*> renderer_pointers;
	for (csri_rend* rp= csri_renderer_default(); rp; rp = csri_renderer_next(rp))
	{
		if (rp)
		{
			csri_info *renderer_info = csri_renderer_info(rp);
			std::string rname(renderer_info->name);
			renderer_names.push_back(rname);
			renderer_pointers.push_back(renderer_info);
		}
	}
	int renderer_count = renderer_names.size(); //No. of renderers available
	//find individual renderer
	if (isSRT)
	{
		bool found = false;
		int i = 0;
		do
		{
			size_t idx = std::string::npos;
			size_t idx_s = std::string::npos;
			idx = renderer_names[i].find("SRT");
			idx_s = renderer_names[i].find("srt");
			if ((idx != std::string::npos)||(idx_s != std::string::npos))
			{
				fh->renderer = renderer_pointers[i];
				found = true;
			}
			i++;
		} while (!found && i<renderer_count);

	}
	else if (isSSA)
	{
		bool found = false;
		int i = 0;
		do
		{
			size_t idx = std::string::npos;
			size_t idx_s = std::string::npos;
			idx = renderer_names[i].find("SSA");
			idx_s = renderer_names[i].find("ssa");
			if ((idx != std::string::npos) || (idx_s != std::string::npos))
			{
				fh->renderer = renderer_pointers[i];
				found = true;
			}
			i++;
		} while (!found && i<renderer_count);
	}
	else //Assume AAS
	{
		bool found = false;
		int i = 0;
		do
		{
			size_t idx = std::string::npos;
			size_t idx_s = std::string::npos;
			idx = renderer_names[i].find("AAS");
			idx_s = renderer_names[i].find("aas");
			if ((idx != std::string::npos) || (idx_s != std::string::npos))
			{
				fh->renderer = renderer_pointers[i];
				found = true;
			}
			i++;
		} while (!found && i<renderer_count);
	}
	*/
	/*
	if (!fh->renderer) //if nothing is found
	{
		fh->renderer = renderer_pointers[0]; //use the default renderer
	}
	*/
	/* Just use the default! No other choice anyway :-) */
	fh->renderer = csri_renderer_default();

	//Convert filename to UTF-16 prior feeding into CSRI
	size_t file_byte_size = strlen(file);
	int charCount = MultiByteToWideChar(CP_THREAD_ACP, NULL, file, file_byte_size, nullptr, 0);
	DWORD cvtError = GetLastError();
	WCHAR* u16buffer = new WCHAR[charCount+1];
	SecureZeroMemory(u16buffer, (charCount+1) * 2);
	CHAR* u8filename = nullptr;
	if (cvtError || !charCount)
	{
		MessageBox(nullptr, "Filename unicode conversion error", "Filename Error", MB_OK);
		if (fh) delete fh;
		return FALSE;
	}
	else
	{
		//u16buffer = new WCHAR[charCount];
		MultiByteToWideChar(CP_THREAD_ACP, NULL, file, file_byte_size, u16buffer, charCount); //to utf-16
		
		int u8byteCount = WideCharToMultiByte(CP_UTF8, NULL, u16buffer, charCount, nullptr, 0, NULL, NULL);
		u8filename = new CHAR[u8byteCount+2];
		SecureZeroMemory(u8filename, u8byteCount+2);
		WideCharToMultiByte(CP_UTF8, NULL, u16buffer, charCount, u8filename, u8byteCount, NULL, NULL);
		//memcpy_s(u16filename, buffersize, u16buffer, buffersize);
		delete[] u16buffer;
	}

	//Initialize track, i.e. open subtilte with CSRI
	fh->track = csri_open_file(fh->renderer, u8filename, NULL);
	delete[] u8filename;
	if (!fh->track)//Fail to open file
	{
		MessageBox(nullptr, "Failed to open file", "CSRI ERROR", MB_OK);
		delete fh;
		return FALSE;
	}
	//Set CSRI Format
	fh->csri_format = new csri_fmt;
	fh->csri_format->width = fh->width;
	fh->csri_format->height = fh->height;
	fh->csri_format->pixfmt = csri_pixfmt::CSRI_F_BGR_; //only BGR_ works
	/*
	//Set style override
	std::regex pStyleOverride("Override\\s*=\\s*(\\d+)$", std::regex_constants::icase);
	std::smatch mStyleOverride;
	int foverride = 0;
	bool isStyleOverride = std::regex_search(INI_content, mStyleOverride, pStyleOverride);
	if (isStyleOverride)
	{
		foverride = std::stoi(mStyleOverride[1]);
	}
	if (foverride) //parse parameters
	{
		//Initialize with defaults
		int BorderStyle = 0;
		int CharSet = 65001;
		DWORD c1st = 0xFFFFFF;
		DWORD c2nd = 0xFFFFFF;
		DWORD cborder = 0xA030A0;
		DWORD cshadow = 0x000000;
		BYTE a1st = 20;
		BYTE a2nd = 0;
		BYTE aborder = 20;
		BYTE ashadow = 255;
		int blur = 5;
		double gaussian = 0.5;
		CString font = "Lucida Sans Unicode";
		int fontsize = 18;
		//Setup regex pattern
		std::regex pBorder_Style("Border_style\\s*=\\s*(\\d+)$", std::regex_constants::icase);
		std::regex pCharSet("CharSet\\s*=\\s*(\\d+)$", std::regex_constants::icase);
		std::regex pCp("Color_primary\\s*=\\s*(0x[0-9,A-F]+)$", std::regex_constants::icase);
		std::regex pCs("Color_secondary\\s*=\\s*(0x[0-9,A-F]+)$", std::regex_constants::icase);
		std::regex pCb("Color_border\\s*=\\s*(0x[0-9,A-F]+)$", std::regex_constants::icase);
		std::regex pCsh("Color_shadow\\s*=\\s*(0x[0-9,A-F]+)$", std::regex_constants::icase);
		std::regex pAp("Alpha_primary\\s*=\\s*(\\d+)$", std::regex_constants::icase);
		std::regex pAs("Alpha_secondary\\s*=\\s*(\\d+)$", std::regex_constants::icase);
		std::regex pAb("Alpha_border\\s*=\\s*(\\d+)$", std::regex_constants::icase);
		std::regex pAsh("Alpha_shadow\\s*=\\s*(\\d+)$", std::regex_constants::icase);
		std::regex pblur("Blur\\s*=\\s*(\\d+)$", std::regex_constants::icase);
		std::regex pgau("Gaussian\\s*=\\s*(\\d+)$", std::regex_constants::icase);
		std::regex pfontsize("Font_size\\s*=\\s*(\\d+)$", std::regex_constants::icase);
		std::regex pfontface("Font_face\\s*=\\s*(.+)$", std::regex_constants::icase);

		//setup storage
		std::smatch mBS, mCS, mCp, mCs, mCb, mCsh, mAp, mAs, mAb, mAsh, mblur, mgau, mfs, mff;
		//search
		bool hasBS=std::regex_search(INI_content, mBS, pBorder_Style);
		bool hasCS = std::regex_search(INI_content, mCS, pCharSet);
		bool hasCp = std::regex_search(INI_content, mCp, pCp);
		bool hasCs = std::regex_search(INI_content, mCs, pCs);
		bool hasCb = std::regex_search(INI_content, mCb, pCb);
		bool hasCsh = std::regex_search(INI_content, mCsh, pCsh);
		bool hasAp = std::regex_search(INI_content, mAp, pAp);
		bool hasAs = std::regex_search(INI_content, mAs, pAs);
		bool hasAb = std::regex_search(INI_content, mAb, pAb);
		bool hasAsh = std::regex_search(INI_content, mAsh, pAsh);
		bool hasblur = std::regex_search(INI_content, mblur, pblur);
		bool hasgau = std::regex_search(INI_content, mgau, pgau);
		bool hasFF = std::regex_search(INI_content, mff, pfontface);
		bool hasFS = std::regex_search(INI_content, mfs, pfontsize);
		//parse
		if (hasBS) BorderStyle = std::stoi(mBS[1]);
		if (hasCS) CharSet = std::stoi(mCS[1]);
		if (hasCp) c1st = std::stoul(mCp[1], nullptr, 16);
		if (hasCs) c2nd = std::stoul(mCs[1], nullptr, 16);
		if (hasCb) cborder = std::stoul(mCb[1], nullptr, 16);
		if (hasCsh) cshadow = std::stoul(mCsh[1], nullptr, 16);
		if (hasAp) a1st = (BYTE)std::stoi(mAp[1]);
		if (hasAs) a2nd = (BYTE)std::stoi(mAs[1]);
		if (hasAb) aborder = (BYTE)std::stoi(mAb[1]);
		if (hasAsh) ashadow = (BYTE)std::stoi(mAsh[1]);
		if (hasblur) blur = std::stoi(mblur[1]);
		if (hasgau) gaussian = std::stod(mgau[1]);
		if (hasFS) fontsize = std::stoi(mfs[1]);
		std::string fontname = mff[1];
		if (hasFF) font = CString(fontname.c_str());

		//call the override function
		fh->style= csri_overide_style(fh->track, BorderStyle, CharSet, c1st, c2nd, cborder, cshadow,
			a1st, a2nd, aborder, ashadow, blur, gaussian,
			font, fontsize);
	}*/
	//Bitmap header?
	fh->bmi = new BITMAPINFOHEADER;
	SecureZeroMemory(fh->bmi, sizeof(BITMAPINFOHEADER));
	fh->bmi->biSize = sizeof(BITMAPINFOHEADER);
	fh->bmi->biBitCount = 32; //reserve a byte for alpha/padding?
	fh->bmi->biCompression = BI_RGB;
	fh->bmi->biHeight = fh->height;
	fh->bmi->biWidth = fh->width;
	fh->bmi->biPlanes = 1;
	fh->bmi->biSizeImage = fh->width * fh->height * 4;
	
	return (INPUT_HANDLE)fh;
}
BOOL func_close(INPUT_HANDLE ih)
{
	//TODO
	SubFileHandle *fh = (SubFileHandle*)ih;
	/*
	if (fh->style)
	{
		csri_overide_cancel(fh->track, fh->style);
	}*/
	if (fh->csri_format)
	{
		delete fh->csri_format;
	}
	if (fh->bmi)
	{
		delete fh->bmi;
	}
	if (fh->track)
	{
		csri_close(fh->track);
	}
	
	delete fh;
	return TRUE;
}
BOOL func_info_get(INPUT_HANDLE ih, INPUT_INFO *iip)
{
	//TODO
	SubFileHandle* fh = (SubFileHandle*)ih;
	iip->flag = INPUT_PLUGIN_FLAG_VIDEO|INPUT_INFO_FLAG_VIDEO_RANDOM_ACCESS;
	iip->rate = 25; //MPC-HC seems fixed this to 25fps
	iip->scale = 1;
	LONGLONG frame_count = fh->duration * iip->rate / 1000;
	if (frame_count > INT32_MAX)
	{
		frame_count = INT32_MAX;
	}
	iip->n = (int)frame_count;
	iip->format = fh->bmi;
	iip->handler = NULL;
	return TRUE;
}
int func_read_video(INPUT_HANDLE ih, int frame, void *buf)
{
	//TODO
	SubFileHandle* fh = (SubFileHandle*)ih;
	csri_frame* this_frame = new csri_frame;
	SecureZeroMemory(this_frame, sizeof(csri_frame));
	this_frame->pixfmt = csri_pixfmt::CSRI_F_BGR_;
	int byte_size = fh->width*fh->height * 4;
	//UCHAR* canvas = new UCHAR[byte_size];
	//UCHAR* whiteboard = new UCHAR[byte_size];
	UCHAR* canvas = nullptr;
	UCHAR* whiteboard = nullptr;
	__try
	{
		canvas = new UCHAR[byte_size];
		whiteboard = new UCHAR[byte_size];
	}
	__except (TRUE)
	{
		MessageBox(nullptr, "Cannot allocate image buffer!", "Subtitle ERROR", MB_OK);
		if (canvas) delete[] canvas;
		if (whiteboard) delete[] whiteboard;
		delete this_frame;
		delete fh->bmi;
		delete fh->csri_format;
		csri_close(fh->track);
		return 0;
	}
	

	SecureZeroMemory(canvas, byte_size); //set to black
	memset(whiteboard, 255, byte_size);//set to white
	this_frame->planes[0] = canvas + (fh->height - 1) * fh->width * 4; //move to RB corner

	this_frame->strides[0] = -(signed)fh->width * 4; //-ve for flipped image

	//calc timestamp from frame no.
	double timestamp_sec = frame / 25.0; //the csri interface use SECONDs!!!
	//render
	int state= csri_request_fmt(fh->track, fh->csri_format);
	if (state == 0)
	{
		csri_render(fh->track, this_frame, timestamp_sec); //render on black background

		//Transparency calc for SRT
		//if (fh->isSRT)
		//{
			//Set planes[0] to white board
			this_frame->planes[0] = whiteboard + (fh->height - 1) * fh->width * 4;
			//render on white background
			__try
			{
				csri_render(fh->track, this_frame, timestamp_sec);
			}
			__except (TRUE)
			{
				MessageBox(nullptr, "Unable to render frame!\nInvalid format probably.", "CSRI ERROR", MB_OK);
				if (canvas) delete[] canvas;
				if (whiteboard) delete[] whiteboard;
				delete this_frame;
				delete fh->bmi;
				delete fh->csri_format;
				csri_close(fh->track);
				return 0;
			}
			int px_count = byte_size;
			
			for (int i = 0; i < px_count; i += 4)
			{   //Alpha channel reconstruction code stolen from AULS's PNG output plugin
				UCHAR aB = (UCHAR)(255 - whiteboard[i] + canvas[i]);
				//if (aB == 0) aB = 1;
				canvas[i] = aB ? (UCHAR)(canvas[i] * 255 / aB) : canvas[i];
				UCHAR aG = (UCHAR)(255 - whiteboard[i + 1] + canvas[i + 1]);
				//if (aG == 0) aG = 1;
				canvas[i + 1] = aG ? (UCHAR)(canvas[i + 1] * 255 / aG) : canvas[i + 1];

				UCHAR aR = (UCHAR)(255 - whiteboard[i + 2] + canvas[i + 2]);
				//if (aR == 0) aR = 1;
				canvas[i + 2] = aR ? (UCHAR)(canvas[i + 2] * 255 / aR) : canvas[i + 2];
				canvas[i + 3] = (UCHAR)((aB + aG + aR) / 3); //alpha value
				

			}
		//}
		
	}
	else
	{
		MessageBox(nullptr, "BGR_ format not supported!", "CSRI ERROR", MB_OK);
	}
	
	//copy to buf
	//size_t data_size = sizeof(this_frame->planes[0]);
	__try
	{
		memcpy_s(buf, byte_size, canvas, byte_size);
	}
	__except (TRUE)
	{
		MessageBox(nullptr, "Cannot send image back to AviUtl!", "Subtitle ERROR", MB_OK);
		if (canvas) delete[] canvas;
		if (whiteboard) delete[] whiteboard;
		delete this_frame;
		delete fh->bmi;
		delete fh->csri_format;
		csri_close(fh->track);
		return 0;
	}
	
	//cleanup
	delete[] whiteboard;
	delete[] canvas;
	delete this_frame;
	//return size
	return byte_size;
}