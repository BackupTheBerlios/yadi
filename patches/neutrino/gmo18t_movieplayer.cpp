/*
  Neutrino-GUI  -   DBoxII-Project

  Movieplayer (c) 2003, 2004 by gagga
  Based on code by Dirch, obi and the Metzler Bros. Thanks.

  $Id: gmo18t_movieplayer.cpp,v 1.1 2004/10/07 12:11:27 essu Exp $

  Homepage: http://www.giggo.de/dbox2/movieplayer.html

  License: GPL

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_DVB_API_VERSION >= 3

#include <gui/movieplayer.h>

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <daemonc/remotecontrol.h>
extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */
#include <system/settings.h>

#include <gui/eventlist.h>
#include <gui/color.h>
#include <gui/infoviewer.h>
#include <gui/nfs.h>
#include <gui/bookmarkmanager.h>
#include <gui/timeosd.h>

#include <gui/widget/buttons.h>
#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>

#include <linux/dvb/audio.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/video.h>

#include <algorithm>
#include <fstream>
#include <sstream>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <transform.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include <poll.h>

#define ADAP	"/dev/dvb/adapter0"
#define ADEC	ADAP "/audio0"
#define VDEC	ADAP "/video0"
#define DMX	ADAP "/demux0"
#define DVR	ADAP "/dvr0"

#define AVIA_AV_STREAM_TYPE_0           0x00
#define AVIA_AV_STREAM_TYPE_SPTS        0x01
#define AVIA_AV_STREAM_TYPE_PES         0x02
#define AVIA_AV_STREAM_TYPE_ES          0x03

#define STREAMTYPE_DVD	1
#define STREAMTYPE_SVCD	2
#define STREAMTYPE_FILE	3

#define MOVIEPLAYER_ConnectLineBox_Width	15

#define RINGBUFFERSIZE 348*188*10
#define MAXREADSIZE 348*188
#define MINREADSIZE 348*188

//TODO: calculate offset for jumping 1 minute forward/backwards in stream
// needs to be a multiplier of 188
// do a VERY shitty approximation here...
//long long minuteoffset = 557892632/2;
//long long minuteoffset = 8807424;
#define MINUTEOFFSET 30898176
	

static CMoviePlayerGui::state g_playstate;
static bool isTS, isPES, isBookmark;
int g_speed = 1;
static off_t g_fileposition;
ringbuffer_t *ringbuf;
bool bufferfilled;
int streamingrunning;
unsigned short pida, pidv;
short ac3;
CHintBox *hintBox;
CHintBox *bufferingBox;
bool avpids_found;
std::string startfilename;
std::string skipvalue;

off_t g_startposition = 0L;
int g_jumpminutes = 1;
int buffer_time = 0;

unsigned short apids[10];
unsigned short ac3flags[10];
unsigned short numpida=0;
unsigned int currentapid = 0, currentac3 = 0, apidchanged=0;
bool showaudioselectdialog = false;


//------------------------------------------------------------------------
void checkAspectRatio (int vdec, bool init);

size_t
CurlDummyWrite (void *ptr, size_t size, size_t nmemb, void *data)
{
	std::string* pStr = (std::string*) data;
	*pStr += (char*) ptr;
	return size * nmemb;
}

//------------------------------------------------------------------------

int CAPIDSelectExec::exec(CMenuTarget* parent, const std::string & actionKey)
{
	apidchanged = 0;
	unsigned int sel= atoi(actionKey.c_str());
	if (currentapid != apids[sel-1] )
	{
		currentapid = apids[sel-1];
		currentac3 = ac3flags[sel-1];
		apidchanged = 1;
        printf("[movieplayer.cpp] apid changed to %d\n",apids[sel-1]);
	}
	return menu_return::RETURN_EXIT;
}

//------------------------------------------------------------------------

CMoviePlayerGui::CMoviePlayerGui()
{
	frameBuffer = CFrameBuffer::getInstance();
	filebrowser = new CFileBrowser ();
	filebrowser->Multi_Select = false;
	filebrowser->Dirs_Selectable = false;
	tsfilefilter.addFilter ("ts");
	vlcfilefilter.addFilter ("mpg");
	vlcfilefilter.addFilter ("mpeg");
	vlcfilefilter.addFilter ("m2p");
	vlcfilefilter.addFilter ("avi");
	vlcfilefilter.addFilter ("vob");
	pesfilefilter.addFilter ("mpv");
	filebrowser->Filter = &tsfilefilter;
	if (strlen (g_settings.network_nfs_moviedir) != 0)
		Path_local = g_settings.network_nfs_moviedir;
	else
		Path_local = "/";
	Path_vlc  = "vlc://";
	Path_vlc += g_settings.streaming_server_startdir;
	Path_vlc_settings = g_settings.streaming_server_startdir;
}

//------------------------------------------------------------------------

CMoviePlayerGui::~CMoviePlayerGui ()
{
	delete filebrowser;
	delete bookmarkmanager;
	g_Zapit->setStandby (false);
	g_Sectionsd->setPauseScanning (false);

}

//------------------------------------------------------------------------
int
CMoviePlayerGui::exec (CMenuTarget * parent, const std::string & actionKey)
{
	printf("[movieplayer.cpp] actionKey=%s\n",actionKey.c_str());
	
	if(Path_vlc_settings != g_settings.streaming_server_startdir)
	{
		Path_vlc  = "vlc://";
		Path_vlc += g_settings.streaming_server_startdir;
		Path_vlc_settings = g_settings.streaming_server_startdir;
	}
	bookmarkmanager = new CBookmarkManager ();

	if (parent)
	{
		parent->hide ();
	}

	bool usedBackground = frameBuffer->getuseBackground();
	if (usedBackground)
	{
		frameBuffer->saveBackgroundImage();
		frameBuffer->ClearFrameBuffer();
	}

	const CBookmark * theBookmark = NULL;
	if (actionKey=="bookmarkplayback") {
		isBookmark = true;
		theBookmark = bookmarkmanager->getBookmark(NULL);
		if (theBookmark == NULL) {
			bookmarkmanager->flush();
			return menu_return::RETURN_REPAINT;
		}
	}
	
	// set zapit in standby mode
	g_Zapit->setStandby (true);

	// tell neutrino we're in ts_mode
	CNeutrinoApp::getInstance ()->handleMsg (NeutrinoMessages::CHANGEMODE,
						 NeutrinoMessages::mode_ts);
	// remember last mode
	m_LastMode =
		(CNeutrinoApp::getInstance ()->
		 getLastMode () | NeutrinoMessages::norezap );

	// Stop sectionsd
	//g_Sectionsd->setPauseScanning (true);

    isBookmark=false;
    startfilename = "";
    g_startposition = 0;
    isTS=false;
    isPES=false;
    
	if (actionKey=="fileplayback") {
        PlayStream (STREAMTYPE_FILE);	
	}
	else if (actionKey=="dvdplayback") {
        PlayStream (STREAMTYPE_DVD);
	}
	else if (actionKey=="vcdplayback") {
        PlayStream (STREAMTYPE_SVCD);
	}
	else if (actionKey=="tsplayback") {
        isTS=true;
        PlayFile();
	}
	else if (actionKey=="pesplayback") {
        //isPES=true;
        ParentalEntrance();
	}
	else if (actionKey=="bookmarkplayback") {
        isBookmark = true;
        if (theBookmark != NULL) {
            startfilename = theBookmark->getUrl();
            sscanf (theBookmark->getTime(), "%lld", &g_startposition);
            int vlcpos = startfilename.rfind("vlc://");
            if (vlcpos==0)
            {
                PlayStream (STREAMTYPE_FILE);	
            }
            else {
                // TODO check if file is a TS. Not required right now as writing bookmarks is disabled for PES anyway
                isTS = true;
                isPES = false;
                PlayFile();
            }
                
        }

	}

	bookmarkmanager->flush();

	// Restore previous background
	if (usedBackground)
	{
		frameBuffer->restoreBackgroundImage();
		frameBuffer->useBackground(true);
		frameBuffer->paintBackground();
	}

	// Restore last mode
	g_Zapit->setStandby (false);

	// Start Sectionsd
	g_Sectionsd->setPauseScanning (false);

	// Restore last mode
	CNeutrinoApp::getInstance ()->handleMsg (NeutrinoMessages::CHANGEMODE,
						 m_LastMode);
	g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );

	CLCD::getInstance()->showServicename(g_RemoteControl->getCurrentChannelName());
	// always exit all
	return menu_return::RETURN_REPAINT;
}

//------------------------------------------------------------------------
CURLcode sendGetRequest (const std::string & url, std::string & response, bool useAuthorization) {
	CURL *curl;
	CURLcode httpres;
  
	curl = curl_easy_init ();
	curl_easy_setopt (curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, CurlDummyWrite);
	curl_easy_setopt (curl, CURLOPT_FILE, (void *)&response);
	if (useAuthorization) curl_easy_setopt (curl, CURLOPT_USERPWD, "admin:admin"); /* !!! make me customizable */
	curl_easy_setopt (curl, CURLOPT_FAILONERROR, true); 
	httpres = curl_easy_perform (curl);
	//printf ("[movieplayer.cpp] HTTP Result: %d\n", httpres);
	curl_easy_cleanup (curl);
	return httpres;
}

//------------------------------------------------------------------------
bool VlcSendPlaylist(char* mrl)
{
	CURLcode httpres;
	std::string baseurl = "http://";
	baseurl += g_settings.streaming_server_ip;
	baseurl += ':';
	baseurl += g_settings.streaming_server_port;
	baseurl += '/';
	
	// empty playlist
	std::string response;
	std::string emptyurl = baseurl;
	emptyurl += "?control=empty";
	httpres = sendGetRequest(emptyurl, response, false);
	printf ("[movieplayer.cpp] HTTP Result (emptyurl): %d\n", httpres);
	if (httpres != 0)
	{
		DisplayErrorMessage(g_Locale->getText(LOCALE_MOVIEPLAYER_NOSTREAMINGSERVER)); // UTF-8
		g_playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
		// Assume safely that all succeeding HTTP requests are successful
	}

	// add MRL
	/* demo MRLs:
	   - DVD: dvdsimple:D:@1:1
	   - DemoMovie: c:\\TestMovies\\dolby.mpg
	   - SVCD: vcd:D:@1:1
	*/
	std::string addurl = baseurl;
	addurl += "?control=add&mrl=";
	addurl += mrl;
	httpres = sendGetRequest(addurl, response, false);
	return (httpres==0);
}
#define TRANSCODE_VIDEO_OFF 0
#define TRANSCODE_VIDEO_MPEG1 1
#define TRANSCODE_VIDEO_MPEG2 2
//------------------------------------------------------------------------
bool VlcRequestStream(int  transcodeVideo, int transcodeAudio)
{
	CURLcode httpres;
	std::string baseurl = "http://";
	baseurl += g_settings.streaming_server_ip;
	baseurl += ':';
	baseurl += g_settings.streaming_server_port;
	baseurl += '/';
	
	// add sout (URL encoded)
	// Example(mit transcode zu mpeg1): ?sout=#transcode{vcodec=mpgv,vb=2000,acodec=mpga,ab=192,channels=2}:duplicate{dst=std{access=http,mux=ts,url=:8080/dboxstream}}
	// Example(ohne transcode zu mpeg1): ?sout=#duplicate{dst=std{access=http,mux=ts,url=:8080/dboxstream}}
	//TODO make this nicer :-)
	std::string souturl;

	//Resolve Resolution from Settings...
	const char * res_horiz;
	const char * res_vert;
	switch (g_settings.streaming_resolution)
	{
		case 0:
			res_horiz = "352";
			res_vert = "288";
			break;
		case 1:
			res_horiz = "352";
			res_vert = "576";
			break;
		case 2:
			res_horiz = "480";
			res_vert = "576";
			break;
		case 3:
			res_horiz = "704";
			res_vert = "576";
			break;
		default:
			res_horiz = "352";
			res_vert = "288";
	} //switch
	souturl = "#";
	if(transcodeVideo!=TRANSCODE_VIDEO_OFF || transcodeAudio!=0)
	{
		souturl += "transcode{";
		if(transcodeVideo!=TRANSCODE_VIDEO_OFF)
		{
			souturl += "vcodec=";
			souturl += (transcodeVideo == TRANSCODE_VIDEO_MPEG1) ? "mpgv" : "mp2v";
			souturl += ",vb=";
			souturl += g_settings.streaming_videorate;
			souturl += ",width=";
			souturl += res_horiz;
			souturl += ",height=";
			souturl += res_vert;
		}
		if(transcodeAudio!=0)
		{
			if(transcodeVideo!=TRANSCODE_VIDEO_OFF)
				souturl += ",";
			souturl += "acodec=mpga,ab=";
			souturl += g_settings.streaming_audiorate;
			souturl += ",channels=2";
		}
		souturl += "}:";
	}
	souturl += "duplicate{dst=std{access=http,mux=ts,url=:";
	souturl += g_settings.streaming_server_port;
	souturl += "/dboxstream}}";
	
	char *tmp = curl_escape (souturl.c_str (), 0);
	printf("[movieplayer.cpp] URL      : %s?sout=%s\n",baseurl.c_str(), souturl.c_str());
	printf("[movieplayer.cpp] URL(enc) : %s?sout=%s\n",baseurl.c_str(), tmp);
	std::string url = baseurl;
	url += "?sout=";
	url += tmp;
	curl_free(tmp);
	std::string response;
	httpres = sendGetRequest(url, response, false);

	// play MRL
	std::string playurl = baseurl;
	playurl += "?control=play&item=0";
	httpres = sendGetRequest(playurl, response, false);

	return true; // TODO error checking
}
//------------------------------------------------------------------------
int VlcGetStreamTime()
{
	// TODO calculate REAL position as position returned by VLC does not take the ringbuffer into consideration
	std::string positionurl = "http://";
	positionurl += g_settings.streaming_server_ip;
	positionurl += ':';
	positionurl += g_settings.streaming_server_port;
	positionurl += "/admin/dboxfiles.html?stream_time=true";
	printf("[movieplayer.cpp] positionurl=%s\n",positionurl.c_str());
	std::string response = "";
	CURLcode httpres = sendGetRequest(positionurl, response, true);
	printf("[movieplayer.cpp] httpres=%d, response.length()=%d, stream_time = %s\n",httpres,response.length(),response.c_str());
	if(httpres== 0 && response.length() > 0)
	{
		return atoi(response.c_str());
	}
	else
		return -1;
}
//------------------------------------------------------------------------
int VlcGetStreamLength()
{
	// TODO calculate REAL position as position returned by VLC does not take the ringbuffer into consideration
	std::string positionurl = "http://";
	positionurl += g_settings.streaming_server_ip;
	positionurl += ':';
	positionurl += g_settings.streaming_server_port;
	positionurl += "/admin/dboxfiles.html?stream_length=true";
	printf("[movieplayer.cpp] positionurl=%s\n",positionurl.c_str());
	std::string response = "";
	CURLcode httpres = sendGetRequest(positionurl, response, true);
	printf("[movieplayer.cpp] httpres=%d, response.length()=%d, stream_length = %s\n",httpres,response.length(),response.c_str());
	if(httpres== 0 && response.length() > 0)
	{
		return atoi(response.c_str());
	}
	else
		return -1;
}
//------------------------------------------------------------------------
void *
ReceiveStreamThread (void *mrl)
{
	printf ("[movieplayer.cpp] ReceiveStreamThread started\n");
	int skt;

	int nothingreceived=0;
	
	// Get Server and Port from Config

	if (!VlcSendPlaylist((char*)mrl))
	{
		DisplayErrorMessage(g_Locale->getText(LOCALE_MOVIEPLAYER_NOSTREAMINGSERVER)); // UTF-8
		g_playstate = CMoviePlayerGui::STOPPED;
		pthread_exit (NULL);
		// Assume safely that all succeeding HTTP requests are successful
	}
	

	int transcodeVideo, transcodeAudio;
	std::string sMRL=(char*)mrl;
	//Menu Option Force Transcode: Transcode all Files, including mpegs.
	if ((!memcmp((char*)mrl, "vcd:", 4) ||
		  !strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "mpg") || 
		  !strcasecmp(sMRL.substr(sMRL.length()-4).c_str(), "mpeg") ||
		  !strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "m2p")))
	{
		if (g_settings.streaming_force_transcode_video)
			transcodeVideo=g_settings.streaming_transcode_video_codec+1;
		else
			transcodeVideo=0;
		transcodeAudio=g_settings.streaming_transcode_audio;
	}
	else
	{
		transcodeVideo=g_settings.streaming_transcode_video_codec+1;
		if((!memcmp((char*)mrl, "dvd", 3) && !g_settings.streaming_transcode_audio) ||
			(!strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "vob") && !g_settings.streaming_transcode_audio) ||
			(!strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "ac3") && !g_settings.streaming_transcode_audio) ||
			g_settings.streaming_force_avi_rawaudio)
			transcodeAudio=0;
		else
			transcodeAudio=1;
	}
	VlcRequestStream(transcodeVideo, transcodeAudio);

// TODO: Better way to detect if http://<server>:8080/dboxstream is already alive. For example repetitive checking for HTTP 404.
// Unfortunately HTTP HEAD requests are not supported by VLC :(
// vlc 0.6.3 and up may support HTTP HEAD requests.

// Open HTTP connection to VLC

	const char *server = g_settings.streaming_server_ip.c_str ();
	int port;
	sscanf (g_settings.streaming_server_port, "%d", &port);

	struct sockaddr_in servAddr;
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons (port);
	servAddr.sin_addr.s_addr = inet_addr (server);

	printf ("[movieplayer.cpp] Server: %s\n", server);
	printf ("[movieplayer.cpp] Port: %d\n", port);
	int len;

	while (true)
	{
		//printf ("[movieplayer.cpp] Trying to call socket\n");
		skt = socket (AF_INET, SOCK_STREAM, 0);

		printf ("[movieplayer.cpp] Trying to connect socket\n");
		if (connect(skt, (struct sockaddr *) &servAddr, sizeof (servAddr)) < 0)
		{
			perror ("SOCKET");
			g_playstate = CMoviePlayerGui::STOPPED;
			pthread_exit (NULL);
		}
		fcntl (skt, O_NONBLOCK);
		printf ("[movieplayer.cpp] Socket OK\n");

		// Skip HTTP header
		const char * msg = "GET /dboxstream HTTP/1.0\r\n\r\n";
		int msglen = strlen (msg);
		if (send (skt, msg, msglen, 0) == -1)
		{
			perror ("send()");
			g_playstate = CMoviePlayerGui::STOPPED;
			pthread_exit (NULL);
		}

		printf ("[movieplayer.cpp] GET Sent\n");

		// Skip HTTP Header
		int found = 0;
		char buf[2];
		char line[200];
		buf[0] = buf[1] = '\0';
		strcpy (line, "");
		while (true)
		{
			len = recv (skt, buf, 1, 0);
			strncat (line, buf, 1);
			if (strcmp (line, "HTTP/1.0 404") == 0)
			{
				printf ("[movieplayer.cpp] VLC still does not send. Retrying...\n");
				close (skt);
				break;
			}
			if ((((found & (~2)) == 0) && (buf[0] == '\r')) || /* found == 0 || found == 2 */
			    (((found & (~2)) == 1) && (buf[0] == '\n')))   /* found == 1 || found == 3 */
			{
				if (found == 3)
					goto vlc_is_sending;
				else
					found++;
			}
			else
			{
				found = 0;
			}
		}
	}
 vlc_is_sending:
	printf ("[movieplayer.cpp] Now VLC is sending. Read sockets created\n");
	hintBox->hide ();
	bufferingBox->paint ();
	printf ("[movieplayer.cpp] Buffering approx. 3 seconds\n");

	int size;
	streamingrunning = 1;
	int fd = open ("/tmp/tmpts", O_CREAT | O_WRONLY);

	struct pollfd poller[1];
	poller[0].fd = skt;
	poller[0].events = POLLIN | POLLPRI;
	int pollret;
	ringbuffer_data_t vec[2];

	while (streamingrunning == 1)
	{
		if (g_playstate == CMoviePlayerGui::STOPPED)
		{
			close(skt);
			pthread_exit (NULL);
		}

		ringbuffer_get_write_vector(ringbuf, &(vec[0]));
		/* vec[0].len is not the total empty size of the buffer! */
		/* but vec[0].len = 0 if and only if the buffer is full! */
		if ((size = vec[0].len) == 0)
		{
			if (avpids_found)
			{
				if (bufferfilled)
				{
					/* do not waste cpu cycles if there is nothing to do */
					usleep(1000);
				}
			}
			else
			{
				printf("[movieplayer.cpp] Searching for vpid and apid\n");
				// find apid and vpid. Easiest way to do that is to write the TS to a file 
				// and use the usual find_avpids function. This is not even overhead as the
				// buffer needs to be prefilled anyway
				close (fd);
				fd = open ("/tmp/tmpts", O_RDONLY);
				//Use global pida, pidv
				//unsigned short pidv = 0, pida = 0;
				find_avpids (fd, &pidv, &pida);
				lseek(fd, 0, SEEK_SET);
				ac3 = (is_audio_ac3(fd) > 0);
				close (fd);
				printf ("[movieplayer.cpp] ReceiveStreamThread: while streaming found pida: 0x%04X ; pidv: 0x%04X ; ac3: %d\n",
					pida, pidv, ac3);
				avpids_found = true;
			}
			if (!bufferfilled)
			{
				bufferingBox->hide ();
				//TODO reset drivers?
				bufferfilled = true;
			}
		}
		else
		{
			//printf("[movieplayer.cpp] ringbuf write space:%d\n",size);

			pollret = poll (poller, (unsigned long) 1, -1);

			if ((pollret < 0) ||
			    ((poller[0].revents & (POLLHUP | POLLERR | POLLNVAL)) != 0))
			{
				perror ("Error while polling()");
				g_playstate = CMoviePlayerGui::STOPPED;
				close(skt);
				pthread_exit (NULL);
			}


			if ((poller[0].revents & (POLLIN | POLLPRI)) != 0)
			{
				len = recv(poller[0].fd, vec[0].buf, size, 0);
			}
			else
				len = 0;

			if (len > 0)
			{
				ringbuffer_write_advance(ringbuf, len);

				nothingreceived = 0;
				//printf ("[movieplayer.cpp] bytes received:%d\n", len);
				if (!avpids_found)
				{
					write (fd, vec[0].buf, len);
				}
			}
			else
			{
				if (g_playstate == CMoviePlayerGui::PLAY) {
					nothingreceived++;
					if (nothingreceived > (buffer_time + 3)*100) // wait at least buffer time secs +3 to play buffer when stream ends
					{
						printf ("[movieplayer.cpp] ReceiveStreamthread: Didn't receive for a while. Stopping.\n");
						g_playstate = CMoviePlayerGui::STOPPED;	
					}
					usleep(10000); //sleep 10 ms
				}
			}
		}
	}
	close(skt);
	pthread_exit (NULL);
}


//------------------------------------------------------------------------
void *
PlayStreamThread (void *mrl)
{
	CURLcode httpres;
	struct dmx_pes_filter_params p;
	ssize_t wr;
	char buf[348 * 188];
	bool failed = false;
	// use global pida and pidv
	pida = 0, pidv = 0, ac3 = -1;
	int done, dmxa=-1 , dmxv = -1, dvr = -1, adec = -1, vdec = -1;

	ringbuf = ringbuffer_create (RINGBUFFERSIZE);
	printf ("[movieplayer.cpp] ringbuffer created\n");

	bufferingBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_MOVIEPLAYER_BUFFERING)); // UTF-8

	std::string baseurl = "http://";
	baseurl += g_settings.streaming_server_ip;
	baseurl += ':';
	baseurl += g_settings.streaming_server_port;
	baseurl += '/';

	printf ("[movieplayer.cpp] mrl:%s\n", (char *) mrl);
	pthread_t rcst;
	pthread_create (&rcst, 0, ReceiveStreamThread, mrl);
	//printf ("[movieplayer.cpp] ReceiveStreamThread created\n");
	if ((dmxa =
	     open (DMX, O_RDWR | O_NONBLOCK)) < 0
	    || (dmxv =
		open (DMX,
		      O_RDWR | O_NONBLOCK)) < 0
	    || (dvr =
		open (DVR,
		      O_WRONLY | O_NONBLOCK)) < 0
	    || (adec =
		open (ADEC,
		      O_RDWR | O_NONBLOCK)) < 0
	    || (vdec = open (VDEC, O_RDWR | O_NONBLOCK)) < 0)
	{
		failed = true;
	}

	g_playstate = CMoviePlayerGui::SOFTRESET;
	printf ("[movieplayer.cpp] read starting\n");
	size_t readsize, len;
	len = 0;
	bool driverready = false;
	std::string pauseurl   = baseurl;
	pauseurl += "?control=pause";
	std::string unpauseurl = pauseurl;
	std::string skipurl;
	std::string response;
	
	checkAspectRatio(vdec, true);
	
	while (g_playstate > CMoviePlayerGui::STOPPED)
	{
		readsize = ringbuffer_read_space (ringbuf);
		if (readsize > MAXREADSIZE)
		{
			readsize = MAXREADSIZE;
		}
		//printf("[movieplayer.cpp] readsize=%d\n",readsize);
		if (bufferfilled)
		{
			if (!driverready)
			{
				driverready = true;
				// pida and pidv should have been set by ReceiveStreamThread now
				printf ("[movieplayer.cpp] PlayStreamthread: while streaming found pida: 0x%04X ; pidv: 0x%04X ac3: %d\n",
					pida, pidv, ac3);

				p.input = DMX_IN_DVR;
				p.output = DMX_OUT_DECODER;
				p.flags = DMX_IMMEDIATE_START;
				p.pid = pida;
				p.pes_type = DMX_PES_AUDIO;
				if (ioctl (dmxa, DMX_SET_PES_FILTER, &p) < 0)
					failed = true;
				p.pid = pidv;
				p.pes_type = DMX_PES_VIDEO;
				if (ioctl (dmxv, DMX_SET_PES_FILTER, &p) < 0)
					failed = true;
				if (ac3 == 1) {
					printf("Setting bypass mode\n");
					if (ioctl (adec, AUDIO_SET_BYPASS_MODE,0UL)<0)
					{
						perror("AUDIO_SET_BYPASS_MODE");
						failed=true;
					}
				}
				else
				{
					ioctl (adec, AUDIO_SET_BYPASS_MODE,1UL);
				}
				if (ioctl (adec, AUDIO_PLAY) < 0)
				{
					perror ("AUDIO_PLAY");
					failed = true;
				}

				if (ioctl (vdec, VIDEO_PLAY) < 0)
				{
					perror ("VIDEO_PLAY");
					failed = true;
				}

				ioctl (dmxv, DMX_START);
				ioctl (dmxa, DMX_START);
				printf ("[movieplayer.cpp] PlayStreamthread: Driver successfully set up\n");
				bufferingBox->hide ();
				// Calculate diffrence between vlc time and play time
				// movieplayer is about to start playback so ask vlc for his position
				if ((buffer_time = VlcGetStreamTime()) < 0)
					buffer_time=0;
			}

			len = ringbuffer_read (ringbuf, buf, (readsize / 188) * 188);

			if (g_startposition > 0) {
			    printf ("[movieplayer.cpp] Was Bookmark. Skipping to startposition\n");
			    char tmpbuf[30];
			    sprintf(tmpbuf,"%lld",g_startposition);
			    skipvalue = tmpbuf;
			    g_startposition = 0;
			    g_playstate = CMoviePlayerGui::SKIP;
			}

			switch (g_playstate)
			{
			case CMoviePlayerGui::PAUSE:
				//ioctl (dmxv, DMX_STOP);
				ioctl (dmxa, DMX_STOP);

				// pause VLC
	            httpres = sendGetRequest(pauseurl, response, false);

				while (g_playstate == CMoviePlayerGui::PAUSE)
				{
					//ioctl (dmxv, DMX_STOP);	
					//ioctl (dmxa, DMX_STOP);
					usleep(100000); // no busy wait
				}
				// unpause VLC
				httpres = sendGetRequest(unpauseurl, response, false);
				g_speed = 1;
				break;
			case CMoviePlayerGui::SKIP:
			{
				skipurl = baseurl;
				skipurl += "?control=seek&seek_value=";
				char * tmp = curl_escape(skipvalue.c_str(), 0);
				skipurl += tmp;
				curl_free(tmp);
				printf("[movieplayer.cpp] skipping URL(enc) : %s\n",skipurl.c_str());
				int bytes = (ringbuffer_read_space(ringbuf) / 188) * 188;
				ringbuffer_read_advance(ringbuf, bytes);
				httpres = sendGetRequest(skipurl, response, false);
//				g_playstate = CMoviePlayerGui::RESYNC;
				g_playstate = CMoviePlayerGui::PLAY;
			}
			break;
			case CMoviePlayerGui::RESYNC:
				printf ("[movieplayer.cpp] Resyncing\n");
				ioctl (dmxa, DMX_STOP);
				printf ("[movieplayer.cpp] Buffering approx. 3 seconds\n");
				/*
				 * always call bufferingBox->paint() before setting bufferfilled to false 
				 * to ensure that it is painted completely before bufferingBox->hide()
				 * might be called by ReceiveStreamThread (otherwise the hintbox's window
				 * variable is deleted while being used)
				 */
				bufferingBox->paint();
				bufferfilled = false;
				ioctl (dmxa, DMX_START);
				g_playstate = CMoviePlayerGui::PLAY;
				break;
			case CMoviePlayerGui::PLAY:
				if (len < MINREADSIZE)
				{
					printf("[movieplayer.cpp] Buffering approx. 3 seconds\n");
					/*
					 * always call bufferingBox->paint() before setting bufferfilled to false 
					 * to ensure that it is painted completely before bufferingBox->hide()
					 * might be called by ReceiveStreamThread (otherwise the hintbox's window
					 * variable is deleted while being used)
					 */
					bufferingBox->paint();
					bufferfilled = false;
			
				}
				//printf ("[movieplayer.cpp] [%d bytes read from ringbuf]\n", len);
				
				done = 0;
				while (len > 0)
				{
					wr = write(dvr, &buf[done], len);
					if (wr < 0)
					{
						if (errno != EAGAIN)
						{
							perror("[movieplayer.cpp] PlayStreamThread: write failed");
#warning I have no clue except what to do if writing fails so I just set playstate to CMoviePlayerGui::STOPPED
							g_playstate = CMoviePlayerGui::STOPPED;
							break;
						}
						else
						{
							usleep(1000);
							continue;
						}
					}
					//printf ("[movieplayer.cpp] [%d bytes written]\n", wr);
					len -= wr;
					done += wr;
				}
				break;
			case CMoviePlayerGui::SOFTRESET:
				ioctl (vdec, VIDEO_STOP);
				ioctl (adec, AUDIO_STOP);
				ioctl (dmxv, DMX_STOP);
				ioctl (dmxa, DMX_STOP);
				ioctl (vdec, VIDEO_PLAY);
				if (ac3 == 1) {
					ioctl (adec, AUDIO_SET_BYPASS_MODE, 0UL );
				}
				else
				{
					ioctl (adec, AUDIO_SET_BYPASS_MODE,1UL);
				}
				ioctl (adec, AUDIO_PLAY);
				p.pid = pida;
				p.pes_type = DMX_PES_AUDIO;
				ioctl (dmxa, DMX_SET_PES_FILTER, &p);
				p.pid = pidv;
				p.pes_type = DMX_PES_VIDEO;
				ioctl (dmxv, DMX_SET_PES_FILTER, &p);
				ioctl (dmxv, DMX_START);
				ioctl (dmxa, DMX_START);
				g_speed = 1;
				g_playstate = CMoviePlayerGui::PLAY;
				break;
			case CMoviePlayerGui::STOPPED:
			case CMoviePlayerGui::PREPARING:
			case CMoviePlayerGui::STREAMERROR:
			case CMoviePlayerGui::FF:
			case CMoviePlayerGui::REW:
			case CMoviePlayerGui::JF:
			case CMoviePlayerGui::JB:
			case CMoviePlayerGui::AUDIOSELECT:
				break;
			}
		}
		else
			usleep(10000); // non busy wait
		
		checkAspectRatio(vdec, false);
	}

	ioctl (vdec, VIDEO_STOP);
	ioctl (adec, AUDIO_STOP);
	ioctl (dmxv, DMX_STOP);
	ioctl (dmxa, DMX_STOP);
	close (dmxa);
	close (dmxv);
	close (dvr);
	close (adec);
	close (vdec);

	// stop VLC
	std::string stopurl = baseurl;
	stopurl += "?control=stop";
	httpres = sendGetRequest(stopurl, response, false);

	printf ("[movieplayer.cpp] Waiting for RCST to stop\n");
	pthread_join (rcst, NULL);
	printf ("[movieplayer.cpp] Seems that RCST was stopped succesfully\n");
  
	// Some memory clean up
	ringbuffer_free(ringbuf);
	delete bufferingBox;
	delete hintBox;
	
	pthread_exit (NULL);
}

//== updateLcd ==
//===============
void updateLcd(const std::string & sel_filename)
{
	char tmp[20];
	std::string lcd;
	
	switch(g_playstate)
	{
	case CMoviePlayerGui::PAUSE:
		lcd = "|| (";
		lcd += sel_filename;
		lcd += ')';
		break;
	case CMoviePlayerGui::REW:
		sprintf(tmp, "%dx<< ", g_speed);
		lcd = tmp;
		lcd += sel_filename;
		break;
	case CMoviePlayerGui::FF:
		sprintf(tmp, "%dx>> ", g_speed);
		lcd = tmp;
		lcd += sel_filename;
		break;
	default:
		lcd = "> ";
		lcd += sel_filename;
		break;
	}
	
	CLCD::getInstance()->showServicename(lcd);
}

// GMO snip start ...

//=============================	
//== PlayTSFile Thread Stuff ==
//=============================
#define PF_BUF_SIZE   (700*188)	// was 360
#define PF_EMPTY      0
#define PF_READY      1
#define PF_LST_ITEMS  30
#define PF_SKPOS_OFFS MINUTEOFFSET  

//-- live stream item --
//----------------------
typedef struct 
{
  std::string        pname;
  std::string        ip;
  int                port;
  int                vpid;
  int                apid;
  long long          zapid;

} MP_LST_ITEM;

//-- main player context --
//-------------------------
typedef struct
{
  pthread_mutex_t mtx0;
  pthread_mutex_t mtx1;
  pthread_mutex_t mtx2;
  pthread_mutex_t mtx3;
  
  pthread_cond_t  cond;
  pthread_t       wThread;
  
  struct dmx_pes_filter_params p;
  short                        ac3;
  unsigned short               pida;
  unsigned short               pidv;

  FILE  *inFp;
  int   inFd;
  int   dvr;
  int   dmxv;
  int   dmxa;
  int   vdec;
  int   adec;
  
  off_t pos;
  off_t fileSize;

  int   readSize;  
  char  buf[4][PF_BUF_SIZE];
  int   r[4];
  int   s[4];

  bool  rdReset;
  bool  wrReset;
  bool  isWriting;
  bool  isReading;

  bool        isStream;
  bool        itChanged;
  bool        canPause;
  int         it;
  int         lst_cnt;
  MP_LST_ITEM lst[PF_LST_ITEMS];

} MP_CTX;

//-- some bullshit global values --
#ifndef _MP_SIMULATOR
static int g_itno = 0;
#endif

static bool openDVBDevices(MP_CTX *ctx);
static void closeDVBDevices(MP_CTX *ctx);
static void PlayTSFileSoftReset(MP_CTX *ctx);
static void PlayTSFileFreezeAV(MP_CTX *ctx, bool resetFlag);
static void PlayTSFileCheckEvent(MP_CTX *ctx);
static bool PlayTSFileWriterCreate(MP_CTX *ctx);
static void *PlayTSFileWriterThread(void *_ctx);
static bool PlayTSFileReadBuffers(MP_CTX *ctx);
static bool PlayTSFileProbe(const char *fname, MP_CTX *ctx);
static void PlayTSFileAnalyze(MP_CTX *ctx);

//== open/close DVB devices ==
//============================
static bool openDVBDevices(MP_CTX *ctx)
{
  //-- may prevent black screen after restart ... --
  //------------------------------------------------
  usleep(150000);
  
  ctx->dmxa = -1;
  ctx->dmxv = -1;
  ctx->dvr  = -1;
  ctx->adec = -1;
  ctx->vdec = -1;

  if ((ctx->dmxa = open(DMX, O_RDWR)) < 0
       || (ctx->dmxv = open(DMX, O_RDWR)) < 0
       || (ctx->dvr = open(DVR, O_WRONLY)) < 0
       || (ctx->adec = open(ADEC, O_RDWR)) < 0 || (ctx->vdec = open(VDEC, O_RDWR)) < 0)
  {  
    return false;
  }
  return true;
}

static void closeDVBDevices(MP_CTX *ctx)
{
  //-- may prevent black screen after restart ... --
  //------------------------------------------------
  usleep(150000);

  if (ctx->vdec != -1) 
  {
    ioctl (ctx->vdec, VIDEO_STOP);
    close (ctx->vdec);
  }
  if (ctx->adec != -1) 
  {
    ioctl (ctx->adec, AUDIO_STOP);
    close (ctx->adec);
  }  
  if (ctx->dmxa != -1) 
  {
    ioctl (ctx->dmxa, DMX_STOP);
    close(ctx->dmxa);
  }
  if (ctx->dmxv != -1) 
  {
    ioctl (ctx->dmxv, DMX_STOP);
    close(ctx->dmxv);
  }
  if (ctx->dvr!= -1) close(ctx->dvr);
 
  ctx->dmxa = -1;
  ctx->dmxv = -1;
  ctx->dvr  = -1;
  ctx->adec = -1;
  ctx->vdec = -1;
}

//== PlayTSFileSoftReset ==
//=========================
static void PlayTSFileSoftReset(MP_CTX *ctx)
{
  //-- don't break active writing --
  while (ctx->isWriting) usleep(5000);
  
  //-- stop DMX devices --
  ioctl(ctx->dmxv, DMX_STOP);
  ioctl(ctx->dmxa, DMX_STOP);

  //-- stop AV devices --
  ioctl(ctx->vdec, VIDEO_STOP);
  ioctl(ctx->adec, AUDIO_STOP);

  //-- setup DMX devices (again) --
  ctx->p.input    = DMX_IN_DVR;
  ctx->p.output   = DMX_OUT_DECODER;
  ctx->p.flags    = 0; //DMX_IMMEDIATE_START;

  ctx->p.pid      = ctx->pida;
  ctx->p.pes_type = DMX_PES_AUDIO;
  ioctl (ctx->dmxa, DMX_SET_PES_FILTER, &(ctx->p));

  ctx->p.pid      = ctx->pidv;
  ctx->p.pes_type = DMX_PES_VIDEO;
  ioctl (ctx->dmxv, DMX_SET_PES_FILTER, &(ctx->p));

  //-- start AV devices again --
  if (ctx->ac3 == 1) 
    ioctl(ctx->adec, AUDIO_SET_BYPASS_MODE,0UL);
  else
    ioctl(ctx->adec, AUDIO_SET_BYPASS_MODE,1UL);

  ioctl(ctx->adec, AUDIO_PLAY);             // audio
  ioctl(ctx->vdec, VIDEO_PLAY);             // video
  ioctl(ctx->adec, AUDIO_SET_AV_SYNC, 1UL); // needs sync !
  
  //-- after buffer reset don't start demuxer here! --
  //-- -> this will be done later in writer thread. --
  if (!ctx->wrReset)
  {
    ioctl(ctx->dmxa, DMX_START);  // audio first !
    ioctl(ctx->dmxv, DMX_START);
  }
}

//== PlayTSFileCheckEvent ==
//==========================
static void PlayTSFileCheckEvent(MP_CTX *ctx)
{
  switch (g_playstate)
  {
    //-- Pause --
    //-----------
    case CMoviePlayerGui::PAUSE:
      //-- live stream won't pause --
      if (!ctx->canPause)
      {
        g_playstate = CMoviePlayerGui::PLAY;
        break;
      }

      //-- Freeze AV with(!) buffer reset --
      PlayTSFileFreezeAV(ctx, true);

      fprintf(stderr, "[mp] pause\n");
      while (g_playstate == CMoviePlayerGui::PAUSE) usleep(10000);
      fprintf(stderr, "[mp] continue\n");
      
      //-- after unpause, a call to "SoftReset" --
      //-- will restart all DVB devices again.  --
      PlayTSFileSoftReset(ctx);
      break;

    //-- next item of program/play-list   --
    //-------------------------------------- 
    case CMoviePlayerGui::ITEMSELECT:
      g_playstate = CMoviePlayerGui::PLAY;

      //-- TODO: may be sometimes will it work for (live) streams --
      if (ctx->isStream) break; // finish
      
      if (ctx->lst_cnt)
      {
        ctx->itChanged = true;
        switch (g_itno)
        {
          //-- previous item --
          case -2:
            ctx->it -= 1;
            if (ctx->it < 0) ctx->it = ctx->lst_cnt-1;
            fprintf(stderr,"[mp] previous item [%d]\n", ctx->it);
            break;

          //-- next item --
          case -1:
            ctx->it += 1;
            if (ctx->it >= ctx->lst_cnt) ctx->it = 0;
            fprintf(stderr, "[mp] next item [%d]\n", ctx->it);
            break;

          //-- directly selected number --
          default:
            if (ctx->it != g_itno)
            {
              if (g_itno >= ctx->lst_cnt)
                ctx->it = ctx->lst_cnt-1;
              else
                ctx->it = g_itno;
              fprintf(stderr, "[mp] selecting item [%d]\n", ctx->it);
            }
            else
              ctx->itChanged = false;
            break;
        }

      }
      // Note: "itChanged" event will cause exit of the main reader
      // loop to select another ts file with full reinitialzing 
      // of the player thread (including FreezeAV/SoftReset) ...
      break;

    //-- not used --
    //--------------
    case CMoviePlayerGui::FF:
    case CMoviePlayerGui::REW:
      g_playstate = CMoviePlayerGui::PLAY;
      break;

    //-- jump forward/back --
    //-----------------------
    case CMoviePlayerGui::JF:
    case CMoviePlayerGui::JB:
      g_playstate = CMoviePlayerGui::PLAY;
      
      //-- (live) stream won't jump via seek --
      if (ctx->isStream) break;

      //-- freeze AV with(!) buffer reset --
      PlayTSFileFreezeAV(ctx, true);
       
      //-- check limits --
      ctx->pos += (g_jumpminutes * MINUTEOFFSET);
      if (ctx->pos >= ctx->fileSize) ctx->pos = ctx->fileSize - PF_SKPOS_OFFS;
      if (ctx->pos < ((off_t)0)) ctx->pos = (off_t)0;
       
      //-- jump to desired file position --  
      lseek (ctx->inFd, ctx->pos, SEEK_SET);
      fprintf(stderr,"[mp] jump to pos (%lld) of total (%lld)\n", 
              ctx->pos, ctx->fileSize);

      //-- Note: the following "SoftReset" call will --
      //-- initiate a restart of all DVB devices.    --
      PlayTSFileSoftReset(ctx);
      break;
     
    //-- soft reset --
    //----------------
    case CMoviePlayerGui::SOFTRESET:
      g_playstate = CMoviePlayerGui::PLAY;
      //PlayTSFileFreezeAV(ctx, false/true???);
      PlayTSFileSoftReset(ctx);
      break;

#ifndef _MP_SIMULATOR
    //-- select audio track --
    //------------------------
    case CMoviePlayerGui::AUDIOSELECT:
      g_playstate = CMoviePlayerGui::PLAY;
      
      //-- (live) stream should have 1 atrack only --
      if (ctx->isStream) break;

      PlayTSFileAnalyze(ctx);
      fprintf(stderr, "[mp] using pida: 0x%04X ; pidv: 0x%04X ; ac3: %d\n",
              ctx->pida, ctx->pidv, ctx->ac3);

      PlayTSFileSoftReset(ctx);
      
      break;
#endif

    //-- other --
    //-----------
    case CMoviePlayerGui::STOPPED:
    case CMoviePlayerGui::PREPARING:
    case CMoviePlayerGui::STREAMERROR:
    case CMoviePlayerGui::PLAY:
    case CMoviePlayerGui::SKIP:
    case CMoviePlayerGui::RESYNC:
#ifdef _MP_SIMULATOR
    case CMoviePlayerGui::AUDIOSELECT:
#endif
      break;
  }

  checkAspectRatio(ctx->vdec, false);
}

//== PlayTSFileWriterCreate ==
//============================
static bool PlayTSFileWriterCreate(MP_CTX *ctx)
{
  for (int i=0; i<4; i++)
  {
    ctx->s[i] = PF_EMPTY;
    ctx->r[i] = 0;
  }
  
  ctx->isWriting = false; // !
  ctx->isReading = true;  // !

  if ( pthread_create(&(ctx->wThread), NULL, PlayTSFileWriterThread, ctx) )
  {  
    fprintf(stderr,"[mp] couldn't create writer thread\n");
    return false;
  }

  return true;
} 

//== PlayTSFileFreezeAV ==
//========================
static void PlayTSFileFreezeAV(MP_CTX *ctx, bool resetFlag)
{
  //-- freeze (pause) AV playback immediate --
  ioctl(ctx->vdec, VIDEO_FREEZE);
  ioctl(ctx->adec, AUDIO_PAUSE);
 
  //-- reset r/w buffers on request -- 
  if (resetFlag)
  {
    for (int i=0; i<4; i++) ctx->s[i] = PF_EMPTY;
  
    ctx->rdReset = true; // !
    ctx->wrReset = true; // !
  }
  
  //-- wait until writer has finished --
  while (ctx->isWriting) usleep(5000);
}

//== PlayTSFileWriterThread ==
//============================
static void *PlayTSFileWriterThread(void *_ctx)
{
  MP_CTX         *ctx = (MP_CTX *)_ctx; 
  unsigned long  cntB = 0;
  unsigned long  cntW = 0;
 
  int  wr;

  fprintf(stderr, "[mp] writer thread starts\n");

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  ctx->isWriting = true;
  while (1)
  {
  
  LB_WR_RESET:
    
    //-- write buffer 0 --
    //--------------------
    pthread_mutex_lock(&(ctx->mtx0));
    {
      if (ctx->s[0] != PF_READY)
      { 
        ctx->isWriting = false;
        cntW++;
        pthread_cond_wait(&(ctx->cond), &(ctx->mtx0));
        ctx->isWriting = true;
      }
      
      //-- after buffer reset, DMX devices has to be started here ! --
      if (ctx->wrReset)
      {
        ctx->wrReset = false;
        ioctl (ctx->dmxa, DMX_START); // audio first !
        ioctl (ctx->dmxv, DMX_START);
      }
      
      if (ctx->s[0] == PF_READY)
      {
        wr = write(ctx->dvr, ctx->buf[0], ctx->r[0]);
        ctx->s[0] = PF_EMPTY;
        cntB++;
      }
      pthread_cond_signal(&ctx->cond);
    }  
    pthread_mutex_unlock(&ctx->mtx0);
    
    if (!ctx->isReading) break; 
    else if (ctx->wrReset) goto LB_WR_RESET;

    //-- write buffer 1 --
    //--------------------
    pthread_mutex_lock(&ctx->mtx1);
    {
      if (ctx->s[1] != PF_READY)
      { 
        ctx->isWriting = false;
        cntW++;
        pthread_cond_wait(&(ctx->cond), &(ctx->mtx1));
        ctx->isWriting = true;
      }

      if (ctx->s[1] == PF_READY)
      {
        wr = write(ctx->dvr, ctx->buf[1], ctx->r[1]);
        ctx->s[1] = PF_EMPTY;
        cntB++;
      }
      pthread_cond_signal(&(ctx->cond));
    }  
    pthread_mutex_unlock(&(ctx->mtx1));
  
    if (!ctx->isReading) break; 
    else if (ctx->wrReset) goto LB_WR_RESET;
    
    //-- write buffer 2 --
    //--------------------
    pthread_mutex_lock(&ctx->mtx2);
    {
      if (ctx->s[2] != PF_READY)
      { 
        ctx->isWriting = false;
        cntW++;
        pthread_cond_wait(&(ctx->cond), &(ctx->mtx2));
        ctx->isWriting = true;
      }
      
      if (ctx->s[2] == PF_READY)
      {
        wr = write(ctx->dvr, ctx->buf[2], ctx->r[2]);
        ctx->s[2] = PF_EMPTY;
        cntB++;
      }
      pthread_cond_signal(&(ctx->cond));
    }  
    pthread_mutex_unlock(&(ctx->mtx2));
  
    if (!ctx->isReading) break; 
    else if (ctx->wrReset) goto LB_WR_RESET;
    
    //-- write buffer 3 --
    //--------------------
    pthread_mutex_lock(&ctx->mtx3);
    {
      if (ctx->s[3] != PF_READY)
      { 
        ctx->isWriting = false;
        cntW++;
        pthread_cond_wait(&(ctx->cond), &(ctx->mtx3));
        ctx->isWriting = true;
      }

      if (ctx->s[3] == PF_READY)
      {
        wr = write(ctx->dvr, ctx->buf[3], ctx->r[3]);
        ctx->s[3] = PF_EMPTY;
        cntB++;
      }
      pthread_cond_signal(&(ctx->cond));
    }  
    pthread_mutex_unlock(&(ctx->mtx3));
  
    if (!ctx->isReading) break; 

  }

  if (!ctx->isReading)
    fprintf(stderr,"[mp] writer: reader exit detected\n");
  
  ctx->isWriting = false;
  fprintf(stderr,"[mp] writer terminated, (%ld) buffers processed, (%ld) waits\n", cntB, cntW);

  pthread_exit(NULL);
}  

//== PlayTSFileReadBuffers ==
//===========================

#if 0
static bool pauseReader(MP_CTX *ctx, pthread_mutex_t *pmtx)
{
  struct timeval  now;
  struct timespec timeout;
  
  gettimeofday(&now, NULL);
  
  timeout.tv_sec  = now.tv_sec + 1;
  timeout.tv_nsec = now.tv_usec * 1000;

  while ( ETIMEDOUT == pthread_cond_timedwait(&(ctx->cond), pmtx, &timeout) )
  {
    if (g_playstate == CMoviePlayerGui::STOPPED) return false;  
  }
  
  return true;
}
#endif 

static bool PlayTSFileReadBuffers(MP_CTX *ctx)
{

LB_RD_RESET:
  ctx->rdReset = false;

  //-- read into buffer 0 --
  //------------------------
  pthread_mutex_lock(&(ctx->mtx0));
  {
    if (ctx->s[0] != PF_EMPTY) pthread_cond_wait(&(ctx->cond),&(ctx->mtx0));
    //if ( (ctx->s[0] != PF_EMPTY) && !pauseReader(ctx, &(ctx->mtx0)) ) 
    //  return false;
  
    if (ctx->inFp)
      ctx->r[0] = fread(ctx->buf[0], 1, ctx->readSize, ctx->inFp);
    else
      ctx->r[0] = read(ctx->inFd, ctx->buf[0], ctx->readSize);
    ctx->pos += ctx->r[0];
    ctx->s[0] = PF_READY;
  }
  pthread_mutex_unlock(&(ctx->mtx0));

  if (ctx->r[0] != ctx->readSize)
  {
    ctx->isReading = false; 
    return false;
  }
  else
  {
    PlayTSFileCheckEvent(ctx);
    if (ctx->rdReset) goto LB_RD_RESET;
  }
  
  //-- read into buffer 1 --
  //------------------------
  pthread_mutex_lock(&(ctx->mtx1));
  {
    if (ctx->s[1] != PF_EMPTY) pthread_cond_wait(&(ctx->cond),&(ctx->mtx1));
    //if ( (ctx->s[1] != PF_EMPTY) && !pauseReader(ctx, &(ctx->mtx1)) ) 
    //  return false;
  
    if (ctx->inFp)
      ctx->r[1] = fread(ctx->buf[1], 1, ctx->readSize, ctx->inFp);
    else
      ctx->r[1] = read(ctx->inFd, ctx->buf[1], ctx->readSize);
    
    ctx->pos += ctx->r[1];
    ctx->s[1] = PF_READY;
    pthread_cond_signal(&(ctx->cond));
  }
  pthread_mutex_unlock(&(ctx->mtx1));

  if (ctx->r[1] != ctx->readSize)
  { 
    ctx->isReading = false; 
    return false;
  }
  else
  {
    PlayTSFileCheckEvent(ctx);
    if (ctx->rdReset) goto LB_RD_RESET;
  }
  
  //-- read into buffer 2 --
  //------------------------
  pthread_mutex_lock(&(ctx->mtx2));
  {
    if (ctx->s[2] != PF_EMPTY) pthread_cond_wait(&(ctx->cond),&(ctx->mtx2));
    //if ( (ctx->s[2] != PF_EMPTY) && !pauseReader(ctx, &(ctx->mtx2)) ) 
    //  return false;
  
    if (ctx->inFp)
      ctx->r[2] = fread(ctx->buf[2], 1, ctx->readSize, ctx->inFp);
    else
      ctx->r[2] = read(ctx->inFd, ctx->buf[2], ctx->readSize);
    ctx->pos += ctx->r[2];
    ctx->s[2] = PF_READY;
  }
  pthread_mutex_unlock(&(ctx->mtx2));

  if (ctx->r[2] != ctx->readSize)
  {
    ctx->isReading = false; 
    return false;
  }
  else
  {
    PlayTSFileCheckEvent(ctx);
    if (ctx->rdReset) goto LB_RD_RESET;
  }
  
  //-- read into buffer 3 --
  //------------------------
  pthread_mutex_lock(&(ctx->mtx3));
  {
    if (ctx->s[3] != PF_EMPTY) pthread_cond_wait(&(ctx->cond), &(ctx->mtx3));
    //if ( (ctx->s[3] != PF_EMPTY) && !pauseReader(ctx, &(ctx->mtx3)) ) 
    //  return false;
  
    if (ctx->inFp)
      ctx->r[3] = fread(ctx->buf[3], 1, ctx->readSize, ctx->inFp);
    else
      ctx->r[3] = read(ctx->inFd, ctx->buf[3], ctx->readSize);
    
    ctx->pos += ctx->r[3];
    ctx->s[3] = PF_READY;
    
    pthread_cond_signal(&(ctx->cond));
  }
  pthread_mutex_unlock(&(ctx->mtx3));

  if (ctx->r[3] != ctx->readSize)
  { 
    ctx->isReading = false; 
    return false;
  }
  else
    PlayTSFileCheckEvent(ctx);

  return true;
}

//== PlayTSFileProbe ==
//=====================
#define MP_STREAM_MAGIC  "#DBOXSTREAM"
#define MP_PLAYLST_MAGIC "#DBOXPLAYLST"

static bool PlayTSFileProbe(const char *fname, MP_CTX *ctx)
{
  ctx->isStream  = false; 
  ctx->itChanged = false;
  ctx->inFp      = NULL;
  ctx->lst_cnt   = 0;
  ctx->it        = 0;

  if ( (ctx->inFp = fopen(fname, "r")) == NULL ) 
    return false; // error 
  
  if (fgets(ctx->buf[0], PF_BUF_SIZE-1, ctx->inFp))
  {
    //-- check first line for magic value --
    if (!memcmp(ctx->buf[0], MP_STREAM_MAGIC, sizeof(MP_STREAM_MAGIC)-1))
    {
      char *s1, *s2;
      int  ntokens;
      
      //-- get all lines (quick and dirty parser) --
      while (fgets(ctx->buf[0], PF_BUF_SIZE-1, ctx->inFp))
      {
        if ( (s2 = strchr(ctx->buf[0],'#')) != NULL ) *s2 = '\0';
        if ( strlen(ctx->buf[0]) < 3 ) continue;

        //-------------------------------------------------------------
        //-- line format:                                            --
        //-- "<progran name>=<ip>;<port>;<vpid>;<apid>;<zapto-id>\n" --
        //-- example:                                                --
        //--   PREMIERE1=dbox;31339;0x1ff;0x2ff;0x20085000a          --
        //-- Note:                                                   -- 
        //--   <zapto-id> can be "0" or "-1" to prevent zapping.     --
        //--    with "-1" pause function will be enabled also.       --
        //-------------------------------------------------------------
        ntokens = 0;        

        //-- program name --
        s1 = ctx->buf[0];
        if ((s2 = strchr(s1, '=')) != NULL )
        {
          ntokens++;
          *s2 = '\0';
          ctx->lst[ctx->lst_cnt].pname = s1;
          s1 = s2+1;
        }   
        
        //-- ip --
        if ((s2 = strchr(s1, ';')) != NULL )
        {
          ntokens++;
          *s2 = '\0';
          ctx->lst[ctx->lst_cnt].ip = s1;
          s1 = s2+1;
        }
        
        //-- port (in dezimal) --
        if ((s2 = strchr(s1, ';')) != NULL )
        {
          ntokens++;
          *s2 = '\0';
          ctx->lst[ctx->lst_cnt].port = strtol(s1, NULL, 10);
          s1 = s2+1;
        }   

        //-- vpid (in hex) --
        if ((s2 = strchr(s1, ';')) != NULL )
        {
          ntokens++;
          *s2 = '\0';
          ctx->lst[ctx->lst_cnt].vpid = strtol(s1, NULL, 16);
          s1 = s2+1;
        }   
        
        //-- apid (in hex) --
        if ((s2 = strchr(s1, ';')) != NULL )
        {
          ntokens++;
          *s2 = '\0';
          ctx->lst[ctx->lst_cnt].apid = strtol(s1, NULL, 16);
          s1 = s2+1;
        }   
        
        //-- sid (in hex) --
        if ((s2 = strchr(s1,'\n')) != NULL ) *s2 = '\0'; 
        
        ntokens++;
        ctx->lst[ctx->lst_cnt].zapid = strtoll(s1, NULL, 16);
          
        if (ntokens == 6)
          ctx->lst_cnt++;
        else
          fprintf(stderr, "[mp] parse error in livestream description\n");
      
        if (ctx->lst_cnt==PF_LST_ITEMS) break; // prevent overflow
      }
      
      if (ctx->lst_cnt) ctx->isStream=true;
    }
    //-- ... or playlist --
    else if (!memcmp(ctx->buf[0], MP_PLAYLST_MAGIC, sizeof(MP_PLAYLST_MAGIC)-1))
    {
      char *s2;
      
      while (fgets(ctx->buf[0], PF_BUF_SIZE-1, ctx->inFp))
      {
        if ( (s2 = strchr(ctx->buf[0],'#')) != NULL ) *s2 = '\0';
        if ( strlen(ctx->buf[0]) < 3 ) continue;

        if ( (s2 = strrchr(ctx->buf[0],'\n')) != NULL ) *s2 = '\0';
        ctx->lst[ctx->lst_cnt].pname = ctx->buf[0];
        ctx->lst_cnt++;
        
        if (ctx->lst_cnt==PF_LST_ITEMS) break; // prevent overflow
      } 
    }
  }

  fclose(ctx->inFp);
  ctx->inFp = NULL;

  if (ctx->lst_cnt)
  {
    fprintf(stderr, "[mp] %s with (%d) entries detected\n",
            (ctx->isStream)?"(live)stream desc":"playlist", ctx->lst_cnt);
  }
    
  return true;  

}

//== PlayTSFileAnalyze ==
//=======================
static void PlayTSFileAnalyze(MP_CTX *ctx)
{
#ifndef _MP_SIMULATOR
  currentapid = 0;  // global
  numpida     = 0;  // global

  find_all_avpids (ctx->inFd, &(ctx->pidv), apids, ac3flags, &numpida);
  lseek(ctx->inFd, ctx->pos, SEEK_SET);
  
  for (int i=0; i<numpida; i++) 
  {
    fprintf(stderr, "[mp] found pida[%d]: 0x%04X, ac3=%d\n", 
    i, apids[i], ac3flags[i]);
  }
  	    
  if (numpida > 1) 
  {
    showaudioselectdialog = true;
    while (showaudioselectdialog) usleep(50000);
  }

  if (!currentapid) 
  {
    ctx->pida   = apids[0];
    ctx->ac3    = ac3flags[0];
    
    currentapid = ctx->pida;
    currentac3  = ctx->ac3;
  }
  else
  {
    ctx->pida   = currentapid;
    ctx->ac3    = currentac3;
  }
  
  apidchanged = 0;
  
#else
  find_avpids (ctx->inFd, &(ctx->pidv), &(ctx->pida));
  lseek(ctx->inFd, 0L, SEEK_SET);
  
  ctx->ac3 = is_audio_ac3(ctx->inFd);
#endif      
}

//== PlayTSFileThread ==
//======================
void *PlayTSFileThread (void *filename)
{
  std::string fname  = (const char *)filename;
  std::string actCmd = "";
  bool        failed = true;

  MP_CTX mpCtx = 
  {   
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_COND_INITIALIZER
  };
  
  MP_CTX *ctx = &mpCtx;
    
  //-- initial setups --
  //--------------------
  ctx->pos = 0L;
  fprintf(stderr, "[mp] PlayFileThread starts\n");

  //-- open DVB devices --
  //----------------------
  if (openDVBDevices(ctx))
  {
    //-- check for valid filename --
    //------------------------------
    if (fname.length())
    {
      //-- check for file type --
      //-------------------------
      if ( PlayTSFileProbe(fname.c_str(), ctx) )
        failed = false;
      else
        fprintf(stderr,"[mp] couldn't open (%s) for probing\n", fname.c_str());
    }
    else
      fprintf(stderr,"[mp] invalid filename (null)\n");
  }
  else  
    fprintf(stderr,"[mp] error opening some DVB devices\n");

  
  //-- terminate on error --
  //------------------------
  if (failed)
  {
    fprintf(stderr, "[mp] PlayTSFileThread terminated\n");
    g_playstate = CMoviePlayerGui::STOPPED;
    closeDVBDevices(ctx);
    pthread_exit(NULL);
  }
  
  do
  {
    //-- (live) stream or ... --
    //--------------------------  
    if (ctx->isStream)
    {
      MP_LST_ITEM *lstIt = &(ctx->lst[ctx->it]);

      ctx->canPause = false;
      
      //-- file stream (timeshift) --
      if (lstIt->zapid < 0)
        ctx->canPause = true;
      //-- tv live stream --
      else if (lstIt->zapid > 0)
      {
        sprintf
        (
          ctx->buf[0],
          "wget -q -O - http://%s/control/zapto?0x%llx",
          lstIt->ip.c_str(), lstIt->zapid
        );
        fprintf(stderr, "[mp] zap: %s\n", ctx->buf[0]);
        system(ctx->buf[0]);
      }
      usleep(250000);
      
      //-- build wget command and store it for later use -- 
      sprintf
      (
        ctx->buf[0], 
        "wget -q -O - http://%s:%d/0x%03x,0x%03x", 
        lstIt->ip.c_str(), lstIt->port, lstIt->vpid, lstIt->apid
      );
      actCmd = ctx->buf[0];  

      //-- start reader pipe --
      if ( (ctx->inFp = popen(ctx->buf[0], "r")) == NULL )
      {
        fprintf(stderr, "error: %s\n", ctx->buf[0]);
        break; // error
      }

      ctx->pidv     = lstIt->vpid;
      ctx->pida     = lstIt->apid;
      ctx->ac3      = 0;
      
      ctx->readSize = PF_BUF_SIZE;
      ctx->fileSize = (off_t)-1;    // is live :-)

    }
    //-- ... plain TS file (playlist) --
    //----------------------------------
    else
    {
      if (ctx->lst_cnt) fname = ctx->lst[ctx->it].pname.c_str();
    
      if ( (ctx->inFd = open(fname.c_str(), O_RDONLY | O_LARGEFILE)) < 0 )
      {
        fprintf(stderr,"[mp] couldn't open (%s)\n", fname.c_str());
        break; // error
      }

      ctx->canPause = true;
      
      //-- set (short) readsize --
      ctx->readSize = PF_BUF_SIZE/2;

      //-- analyze TS file (set apid/vpid/ac3 in ctx) --
      PlayTSFileAnalyze(ctx);
  
      //-- get file size --
      ctx->fileSize = lseek (ctx->inFd, 0L, SEEK_END);

      //-- set bookmark position --
      ctx->pos = g_startposition;
      lseek (ctx->inFd, ctx->pos, SEEK_SET);
      
      g_startposition = 0;
    }
  
    fprintf
    (
      stderr, 
      "[mp] %s with vpid=(0x%04X) apid=(0x%04X) ac3=(%d)\n",
      (ctx->isStream)?"(live) stream":"plain TS file",
      ctx->pidv, ctx->pida, ctx->ac3
    );

    //-- start writer thread (will wait until buffers'r ready) --
    //-----------------------------------------------------------
    if ( PlayTSFileWriterCreate(ctx) )
    {
      unsigned long cntL = 0;

      //-- DVB devices softreset --
      //---------------------------
      PlayTSFileSoftReset(ctx);

      //-- aspect ratio init --
      checkAspectRatio(ctx->vdec, true);

      //-- reader loop --
      //-----------------
      fprintf(stderr,"[mp] entering reader loop\n");
      while ( PlayTSFileReadBuffers(ctx) &&
              (ctx->itChanged == false) &&
              (g_playstate >= CMoviePlayerGui::PLAY) )
      {
        cntL++;
        g_fileposition = ctx->pos;
      } 
     
      //-- kill live stream process (wget) --
      //-------------------------------------
      if (ctx->inFp)
      {
        actCmd = "killall -TERM " + actCmd.substr(0,actCmd.find(' '));
        system(actCmd.c_str());
      }
      
      fprintf(stderr,"[mp] leaving reader loop after (%ld) turns\n", cntL);

      //-- force end of writer thread --
      //--------------------------------
      ctx->isReading = false;
      pthread_cond_signal(&(ctx->cond)); // wake up ...
      pthread_join(ctx->wThread, NULL);  // ... and wait.

      fprintf(stderr,"[mp] reader terminated (last pos=%ldM)\n", 
              (unsigned long)(ctx->pos/(1024*1024)));
    } 

    //-- close input --
    //-----------------
    if (ctx->inFp) 
    { 
      fclose(ctx->inFp);
      ctx->inFp = NULL;
    }
    else if (ctx->inFd != -1)
    {
      //-- eventually activate autoplay for next file in a playlist --
      if ( (ctx->itChanged == false) && ctx->lst_cnt ) 
      {
        ctx->it++;
        if (ctx->it < ctx->lst_cnt) ctx->itChanged = true;
      }
      
      close(ctx->inFd);
      ctx->inFd = -1;
    }
    
    //-- Note: on any content change AV should be freezed first,   --
    //-- to get a consitant state. restart of all DVB-devices will --
    //-- be initiated by a SoftReset in following turns/starts.    --
    PlayTSFileFreezeAV(ctx, true);

    //-- check for another item to play --
    //------------------------------------
    if (g_playstate >= CMoviePlayerGui::PLAY && ctx->itChanged)
    {
      ctx->itChanged = false;
      g_playstate    = CMoviePlayerGui::PLAY;

      //updateLcd(ctx->lst[ctx->it].pname);
    }
    else
      break; // normal loop exit

  } while (1);
 
  //-- stop and close DVB devices --
  //--------------------------------
  closeDVBDevices(ctx);

  if (g_playstate != CMoviePlayerGui::STOPPED)
  {
    g_playstate = CMoviePlayerGui::STOPPED;
    g_RCInput->postMsg (CRCInput::RC_red, 0);	// for faster exit in PlayStream(); do NOT remove!
  }

  fprintf(stderr, "[mp] PlayTSFileThread terminated\n");
  pthread_exit (NULL);
}

//=======================================
//== CMoviePlayerGui::ParentalEntrance ==
//=======================================
#define CI_MAX 4

void CMoviePlayerGui::ParentalEntrance(void)
{
  neutrino_msg_t      msg;
  neutrino_msg_data_t data;

  char code1[CI_MAX+1];
  char code2[CI_MAX+1];
  bool done = false;
  int  ci   = 0;
  
  g_playstate = CMoviePlayerGui::STOPPED;
  do
  {
    //-- check RC code --
    //-------------------
    g_RCInput->getMsg (&msg, &data, 10);  // 1 secs..
    switch(msg)
    {
      case CRCInput::RC_0:
      case CRCInput::RC_1:
      case CRCInput::RC_2:
      case CRCInput::RC_3:
      case CRCInput::RC_4:
      case CRCInput::RC_5:
      case CRCInput::RC_6:
      case CRCInput::RC_7:
      case CRCInput::RC_8:
      case CRCInput::RC_9:
        code2[ci++] = msg - CRCInput::RC_1 + '1';
        
        if (ci>=CI_MAX)
        {
          int  n   = 0;
          FILE *fp = NULL;

          //-- get reference code from file (or set default "2222") -- 
          if ( (fp = fopen("/var/tuxbox/config/mpcode","r")) != NULL )
          {
            n = fread(code1, 1, CI_MAX, fp);
            fclose(fp);
          }
          if (n != CI_MAX) strcpy(code1,"2222");
          
          code1[CI_MAX] = '\0';
          code2[CI_MAX] = '\0';
          if (!strcmp(code1, code2))
          {
            fprintf(stderr, "[mp] pass ok!\n"); 
            PlayFile(1);  
          }
          else
          {  
            //-- TODO: message box (see movieplayer help) --  
            fprintf(stderr, "[mp] pass failed\n"); 
          }
          done = true;
        }
        break;

      //-- Exit for Record/Zapto Timers  etc. --
      case NeutrinoMessages::RECORD_START:
      case NeutrinoMessages::ZAPTO:
      case NeutrinoMessages::STANDBY_ON:
      case NeutrinoMessages::SHUTDOWN:
      case NeutrinoMessages::SLEEPTIMER:
        g_RCInput->postMsg (msg, data);
        done = true;
        break;
 
      default:
        if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
          done = true;
        break;
    }
  }
  while ( done==false );
}

#ifndef _MP_SIMULATOR
//===============================
//== CMoviePlayerGui::PlayFile ==
//===============================
int CMoviePlayerGui::lastParental = -1;

void CMoviePlayerGui::PlayFile (int parental)
{
  neutrino_msg_t      msg;
  neutrino_msg_data_t data;

  std::string sel_filename;
  std::string hlpstr;

  CTimeOSD    FileTime;
  bool        update_lcd       = true;
  bool        open_filebrowser = true;
  bool        start_play       = false;
  bool        done             = false;
  bool        rc_blocked       = false;
  
  g_playstate = CMoviePlayerGui::STOPPED;
  /* g_playstate == CMoviePlayerGui::STOPPED         : stopped
   * g_playstate == CMoviePlayerGui::PLAY            : playing
   * g_playstate == CMoviePlayerGui::PAUSE           : pause-mode
   * g_playstate == CMoviePlayerGui::FF              : fast-forward
   * g_playstate == CMoviePlayerGui::REW             : rewind
   * g_playstate == CMoviePlayerGui::SOFTRESET       : softreset without clearing buffer (g_playstate toggle to 1)
   */

  //-- switch parental access via user definable script --
  if (parental != lastParental)
  {
    fprintf(stderr,"[mp] setting parental to (%d)\n", parental);
    lastParental = parental;

    hlpstr = "/var/bin/parental.sh";
    if (parental==1) hlpstr += " 1"; else hlpstr += " 0";
    system(hlpstr.c_str());
  }
	 
  do
  {
    //-- bookmark playback --
    //-----------------------
    if (isBookmark) 
    {
      open_filebrowser = false;
      filename         = startfilename.c_str();
      sel_filename     = startfilename;
      update_lcd       = true;
      start_play       = true;
      isBookmark       = false;
    }

    //-- file browser --
    //------------------
    if (open_filebrowser)
    {
      open_filebrowser    = false;
      filename            = NULL;
      filebrowser->Filter = &tsfilefilter;

      if (filebrowser->exec(Path_local))
      {
        Path_local = filebrowser->getCurrentDir();
        CFile *file;
    
        if ((file = filebrowser->getSelectedFile()) != NULL)
        {
          filename     = file->Name.c_str();
          update_lcd   = true;
          start_play   = true;
          sel_filename = filebrowser->getSelectedFile()->getFileName();
        }
      }
      else
      {
        if (g_playstate == CMoviePlayerGui::STOPPED) break;
      }

      CLCD::getInstance()->setMode (CLCD::MODE_TVRADIO);
    }

    //-- LCD display --
    //-----------------
    if (update_lcd)
    {
      update_lcd = false;
      updateLcd(sel_filename);
    }

    //-- start threaded playback --
    //-----------------------------
    if (start_play)
    {
      fprintf(stderr, "[mp] Startplay\n");
      start_play = false;

      if (g_playstate >= CMoviePlayerGui::PLAY)
      {
        g_playstate = CMoviePlayerGui::STOPPED;
        pthread_join (rct, NULL);
      }

      g_playstate = CMoviePlayerGui::PLAY;  // !!!

      //-- create player thread --
      if (pthread_create(&rct, 0, PlayTSFileThread, (void *)filename) != 0)
      {
        fprintf(stderr, "[mp] couldn't create player thread\n");
        break;
      }

    }

    //-- audio track selector                  --
    // input:  numpida                         --
    //         ac3flags[numpida]               --
    //         currentapid = 0                 --
    // ouptut: currentapid (may be remain 0,   --
    //           if nothing happened)          -- 
    //         currentac3                      --
    //         apidchanged (0/1, but not used) --
    //-------------------------------------------
    if (showaudioselectdialog) 
    {
      CMenuWidget APIDSelector(LOCALE_APIDSELECTOR_HEAD, "audio.raw", 300);
      APIDSelector.addItem(GenericMenuSeparator);
      apidchanged = 0;

      CAPIDSelectExec *APIDChanger = new CAPIDSelectExec;

      for( unsigned int count=0; count<numpida; count++ )
      {
        char apidnumber[3];
        sprintf(apidnumber, "%d", count+1);

        std::string apidtitle = "Stream ";
        apidtitle.append(apidnumber);
  
        if (ac3flags[count]) apidtitle.append(" (AC3)");
              
         APIDSelector.addItem
         (
           new CMenuForwarderNonLocalized
           (
             apidtitle.c_str(), true, NULL, APIDChanger, apidnumber, 
             CRCInput::convertDigitToKey(count+1)
           ), 
           (count == 0)
         );
      }

      APIDSelector.exec(NULL, "");
      showaudioselectdialog = false;
    }

    //-- filetime display -- 
    //----------------------
    if(FileTime.IsVisible())
    {
      FileTime.update();
    }

    //-- check RC code --
    //-------------------
    g_RCInput->getMsg (&msg, &data, 10);  // 1 secs..
    switch(msg)
    {
      //-- exit play --
      case CRCInput::RC_red:
      case CRCInput::RC_home:
        done = true;
        break;

      //-- pause / play --
      case CRCInput::RC_yellow:
        if (rc_blocked == false) // prevent to fast repeats
        {
          update_lcd  = true;
          //g_playstate = (g_playstate == CMoviePlayerGui::PAUSE) ? CMoviePlayerGui::SOFTRESET : CMoviePlayerGui::PAUSE;
          g_playstate = (g_playstate == CMoviePlayerGui::PAUSE) ? CMoviePlayerGui::PLAY : CMoviePlayerGui::PAUSE;
          rc_blocked  = true;
        }
        break;

      //-- invoke bookmark manager --
      case CRCInput::RC_blue:
        if (bookmarkmanager->getBookmarkCount() < bookmarkmanager->getMaxBookmarkCount())
        {
          char timerstring[200];
          fprintf(stderr, "fileposition: %lld\n",g_fileposition);
          sprintf(timerstring, "%lld",g_fileposition);
          fprintf(stderr, "timerstring: %s\n",timerstring);
          std::string bookmarktime = "";
          bookmarktime.append(timerstring);
          fprintf(stderr, "bookmarktime: %s\n",bookmarktime.c_str());
          bookmarkmanager->createBookmark(filename, bookmarktime);
        }
        else
        {
          fprintf(stderr, "too many bookmarks\n");
          DisplayErrorMessage(g_Locale->getText(LOCALE_MOVIEPLAYER_TOOMANYBOOKMARKS)); // UTF-8
        }
        break;

      //-- request for audio selector --
      case CRCInput::RC_green:
        g_playstate = CMoviePlayerGui::AUDIOSELECT;
        break;

      //-- Help --
      case CRCInput::RC_help:
        hlpstr = g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP);
        hlpstr += "\nVersion: $Revision: 1.1 $\n\nMovieplayer (c) 2003, 2004 by gagga";
        ShowMsgUTF(LOCALE_MESSAGEBOX_INFO, hlpstr.c_str(), CMessageBox::mbrBack, CMessageBox::mbBack, "info.raw"); // UTF-8
        break;

      //-- filetime on/off --
      case CRCInput::RC_setup:
        if(FileTime.IsVisible())
          FileTime.hide();
        else
          FileTime.show(g_fileposition / (MINUTEOFFSET/60));
        break;

      //-- rewind & fast forward --
      case CRCInput::RC_left:
      case CRCInput::RC_right:
        break;

      //-- Softreset --
      case CRCInput::RC_0:
        if (g_playstate != CMoviePlayerGui::PAUSE)
          g_playstate = CMoviePlayerGui::SOFTRESET;
        break;

      //-- jump 1 minute back --
      case CRCInput::RC_1:
        g_jumpminutes = -1;
        g_playstate   = CMoviePlayerGui::JB;
        update_lcd    = true;
        FileTime.hide();
        break;

      //-- jump 1 minute forward --    
      case CRCInput::RC_3:
        g_jumpminutes = 1;
        g_playstate   = CMoviePlayerGui::JF;
        update_lcd    = true;
        FileTime.hide();
        break;

      //-- jump 5 minutes back --
      case CRCInput::RC_4:
        g_jumpminutes = -5;
        g_playstate = CMoviePlayerGui::JB;
        update_lcd = true;
        FileTime.hide();
        break;

      //-- jump 5 minutes forward --
      case CRCInput::RC_6:
        g_jumpminutes = 5;
        g_playstate   = CMoviePlayerGui::JF;
        update_lcd    = true;
        FileTime.hide();
        break;

      //-- jump 10 minutes back -- 
      case CRCInput::RC_7:
        g_jumpminutes = -10;
        g_playstate   = CMoviePlayerGui::JB;
        update_lcd    = true;
        FileTime.hide();
        break;

      //-- jump 10 minutes back --
      case CRCInput::RC_9:
        g_jumpminutes = 10;
        g_playstate   = CMoviePlayerGui::JF;
        update_lcd    = true;
        FileTime.hide();
        break;

      //-- next/prev item --  
      case CRCInput::RC_up:
        g_playstate = CMoviePlayerGui::ITEMSELECT;
        g_itno      = -2;
        break;

      case CRCInput::RC_down:
        g_playstate = CMoviePlayerGui::ITEMSELECT;
        g_itno      = -1;
        break;

      case CRCInput::RC_ok:
        if (g_playstate > CMoviePlayerGui::PLAY)
        {
          update_lcd = true;
          g_playstate = CMoviePlayerGui::SOFTRESET;
        }
        else
          open_filebrowser = true;
        break;

      case NeutrinoMessages::RECORD_START:
      case NeutrinoMessages::ZAPTO:
      case NeutrinoMessages::STANDBY_ON:
      case NeutrinoMessages::SHUTDOWN:
      case NeutrinoMessages::SLEEPTIMER:
        // Exit for Record/Zapto Timers
        fprintf(stderr,"[mp] teminating due to high-prio event\n");
        done = true;
        g_RCInput->postMsg (msg, data);
        break;
 
      default:
        if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
        {
          fprintf(stderr, "[mp] terminating due to cancel_all request\n");  
          done = true;
        }

        rc_blocked = false;   
        break;
    }
  }
  while ( (g_playstate >= CMoviePlayerGui::PLAY) && (done==false) );
  
  g_playstate = CMoviePlayerGui::STOPPED;

  pthread_join (rct, NULL);
}
#endif

// ... GMO snip end


//=================================
//== CMoviePlayerGui::PlayStream ==
//=================================
#define SKIPPING_DURATION 3
void
CMoviePlayerGui::PlayStream (int streamtype)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	std::string sel_filename;
	bool update_info = true, start_play = false, exit =
		false, open_filebrowser = true;
	char mrl[200];
	CTimeOSD StreamTime;

	if (streamtype == STREAMTYPE_DVD)
	{
		strcpy (mrl, "dvdsimple:");
		strcat (mrl, g_settings.streaming_server_cddrive);
		strcat (mrl, "@1:1");
		printf ("[movieplayer.cpp] Generated MRL: %s\n", mrl);
		sel_filename = "DVD";
		open_filebrowser = false;
		start_play = true;
	}
	else if (streamtype == STREAMTYPE_SVCD)
	{
		strcpy (mrl, "vcd:");
		strcat (mrl, g_settings.streaming_server_cddrive);
		strcat (mrl, "@1:1");
		printf ("[movieplayer.cpp] Generated MRL: %s\n", mrl);
		sel_filename = "(S)VCD";
		open_filebrowser = false;
		start_play = true;

	}

	g_playstate = CMoviePlayerGui::STOPPED;
	/* playstate == CMoviePlayerGui::STOPPED         : stopped
	 * playstate == CMoviePlayerGui::PREPARING       : preparing stream from server
	 * playstate == CMoviePlayerGui::ERROR           : error setting up server
	 * playstate == CMoviePlayerGui::PLAY            : playing
	 * playstate == CMoviePlayerGui::PAUSE           : pause-mode
	 * playstate == CMoviePlayerGui::FF              : fast-forward
	 * playstate == CMoviePlayerGui::REW             : rewind
	 * playstate == CMoviePlayerGui::JF              : jump forward x minutes
	 * playstate == CMoviePlayerGui::JB              : jump backward x minutes
	 * playstate == CMoviePlayerGui::SOFTRESET       : softreset without clearing buffer (playstate toggle to 1)
	 */
	do
	{
		if (exit)
		{
			exit = false;
			if (g_playstate >= CMoviePlayerGui::PLAY)
			{
				g_playstate = CMoviePlayerGui::STOPPED;
				break;
			}
		}

		if (isBookmark) {
    	    open_filebrowser = false;
    	    isBookmark = false;
    	    filename = startfilename.c_str();
			int namepos = startfilename.rfind("vlc://");
			std::string mrl_str = startfilename.substr(namepos + 6);
			char *tmp = curl_escape (mrl_str.c_str (), 0);
			strncpy (mrl, tmp, sizeof (mrl) - 1);
			curl_free (tmp);
			printf ("[movieplayer.cpp] Generated Bookmark FILE MRL: %s\n", mrl);
    	    // TODO: What to use for LCD? Bookmarkname? Filename? 
    	    sel_filename = "Bookmark Playback";
    	    update_info=true;
    	    start_play=true;
		}
		
		if (open_filebrowser)
		{
			open_filebrowser = false;
			filename = NULL;
			filebrowser->Filter = &vlcfilefilter;
			if (filebrowser->exec (Path_vlc))
			{
				Path_vlc = filebrowser->getCurrentDir ();
				CFile * file;
				if ((file = filebrowser->getSelectedFile()) != NULL)
				{
					filename = file->Name.c_str();
					sel_filename = file->getFileName();
					//printf ("[movieplayer.cpp] sel_filename: %s\n", filename);
					int namepos = file->Name.rfind("vlc://");
					std::string mrl_str = file->Name.substr(namepos + 6);
					char *tmp = curl_escape (mrl_str.c_str (), 0);
					strncpy (mrl, tmp, sizeof (mrl) - 1);
					curl_free (tmp);
					printf ("[movieplayer.cpp] Generated FILE MRL: %s\n", mrl);

					update_info = true;
					start_play = true;
				}
			}
			else
			{
				if (g_playstate == CMoviePlayerGui::STOPPED)
					break;
			}

			CLCD::getInstance ()->setMode (CLCD::MODE_TVRADIO);
		}

		if (update_info)
		{
			update_info = false;
			updateLcd(sel_filename);
		}

		if (start_play)
		{
			start_play = false;
			bufferfilled = false;
			avpids_found=false;
	  
			if (g_playstate >= CMoviePlayerGui::PLAY)
			{
				g_playstate = CMoviePlayerGui::STOPPED;
				pthread_join (rct, NULL);
			}
			//TODO: Add Dialog (Remove Dialog later)
			hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_MOVIEPLAYER_PLEASEWAIT)); // UTF-8
			hintBox->paint();
			buffer_time=0;
			if (pthread_create (&rct, 0, PlayStreamThread, (void *) mrl) != 0)
			{
				break;
			}
			g_playstate = CMoviePlayerGui::SOFTRESET;
		}

		g_RCInput->getMsg (&msg, &data, 10);	// 1 secs..
		if(StreamTime.IsVisible())
		{
			StreamTime.update();
		}
		if (msg == CRCInput::RC_home || msg == CRCInput::RC_red)
		{
			//exit play
			exit = true;
		}
		else if (msg == CRCInput::RC_yellow)
		{
			update_info = true;
			g_playstate = (g_playstate == CMoviePlayerGui::PAUSE) ? CMoviePlayerGui::SOFTRESET : CMoviePlayerGui::PAUSE;
			StreamTime.hide();
		}
		else if (msg == CRCInput::RC_green)
		{
			if (g_playstate == CMoviePlayerGui::PLAY) g_playstate = CMoviePlayerGui::RESYNC;
			StreamTime.hide();
		}
		else if (msg == CRCInput::RC_blue)
		{
			if (bookmarkmanager->getBookmarkCount() < bookmarkmanager->getMaxBookmarkCount()) 
			{
				int stream_time;
    			if ((stream_time=VlcGetStreamTime()) >= 0)
				{
					std::stringstream stream_time_ss;
					stream_time_ss << (stream_time - buffer_time);
					bookmarkmanager->createBookmark(filename, stream_time_ss.str());
    			}
    			else {
        			DisplayErrorMessage(g_Locale->getText(LOCALE_MOVIEPLAYER_WRONGVLCVERSION)); // UTF-8
    			}
			}
			else {
    			//popup error message
    			printf("too many bookmarks\n");
    			DisplayErrorMessage(g_Locale->getText(LOCALE_MOVIEPLAYER_TOOMANYBOOKMARKS)); // UTF-8
			}
		}
		
		else if (msg == CRCInput::RC_1)
		{
			skipvalue = "-00:01:00";
			g_playstate = CMoviePlayerGui::SKIP;
			StreamTime.hide();
		}
		else if (msg == CRCInput::RC_3)
		{
			skipvalue = "+00:01:00";
			g_playstate = CMoviePlayerGui::SKIP;
			StreamTime.hide();
		}
		else if (msg == CRCInput::RC_4)
		{
			skipvalue = "-00:05:00";
			g_playstate = CMoviePlayerGui::SKIP;
			StreamTime.hide();
		}
		else if (msg == CRCInput::RC_6)
		{
			skipvalue = "+00:05:00";
			g_playstate = CMoviePlayerGui::SKIP;
			StreamTime.hide();
		}
		else if (msg == CRCInput::RC_7)
		{
			skipvalue = "-00:10:00";
			g_playstate = CMoviePlayerGui::SKIP;
			StreamTime.hide();
		}
		else if (msg == CRCInput::RC_9)
		{
			skipvalue = "+00:10:00";
			g_playstate = CMoviePlayerGui::SKIP;
			StreamTime.hide();
		}
		else if (msg == CRCInput::RC_down)
		{
			char tmp[10+1];
			bool cancel;

			CTimeInput ti(LOCALE_MOVIEPLAYER_GOTO, tmp, LOCALE_MOVIEPLAYER_GOTO_H1, LOCALE_MOVIEPLAYER_GOTO_H2, NULL, &cancel);
			ti.exec(NULL, "");
			if(!cancel) // no cancel
			{
				skipvalue = tmp;
				if(skipvalue[0]== '=')
					skipvalue = skipvalue.substr(1);
				g_playstate = CMoviePlayerGui::SKIP;
				StreamTime.hide();
			}
		}
		else if (msg == CRCInput::RC_setup)
 		{
			if(StreamTime.IsVisible())
			{
				if(StreamTime.GetMode() == CTimeOSD::MODE_ASC)
				{
					int stream_length = VlcGetStreamLength();
					int stream_time = VlcGetStreamTime();
					if (stream_time >=0 && stream_length >=0)
					{
						StreamTime.SetMode(CTimeOSD::MODE_DESC);
						StreamTime.show(stream_length - stream_time + buffer_time);
					}
					else
						StreamTime.hide();
				}
				else
					StreamTime.hide();
			}
			else
			{
				int stream_time;
				if ((stream_time = VlcGetStreamTime())>=0)
				{
					StreamTime.SetMode(CTimeOSD::MODE_ASC);
					StreamTime.show(stream_time-buffer_time);
				}
			}
 		}
		else if (msg == CRCInput::RC_help)
 		{
			std::string fullhelptext = g_Locale->getText(LOCALE_MOVIEPLAYER_VLCHELP);
			fullhelptext += "\nVersion: $Revision: 1.1 $\n\nMovieplayer (c) 2003, 2004 by gagga, gmo18t";
			ShowMsgUTF(LOCALE_MESSAGEBOX_INFO, fullhelptext.c_str(), CMessageBox::mbrBack, CMessageBox::mbBack, "info.raw"); // UTF-8
 		}
		else
			if (msg == NeutrinoMessages::RECORD_START
			    || msg == NeutrinoMessages::ZAPTO
			    || msg == NeutrinoMessages::STANDBY_ON
			    || msg == NeutrinoMessages::SHUTDOWN
			    || msg == NeutrinoMessages::SLEEPTIMER)
			{
				// Exit for Record/Zapto Timers
				exit = true;
				g_RCInput->postMsg (msg, data);
			}
			else
				if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
				{
					exit = true;
				}
	}
	while (g_playstate >= CMoviePlayerGui::PLAY);
	pthread_join (rct, NULL);
}


// checks if AR has changed an sets cropping mode accordingly (only video mode auto)
void checkAspectRatio (int vdec, bool init)
{

	static video_size_t size; 
	static time_t last_check=0;
	
	// only necessary for auto mode, check each 5 sec. max
	if(g_settings.video_Format != 0 || (!init && time(NULL) <= last_check+5)) 
		return;

	if(init)
	{
		if (ioctl(vdec, VIDEO_GET_SIZE, &size) < 0)
			perror("[movieplayer.cpp] VIDEO_GET_SIZE");
		last_check=0;
		return;
	}
	else
	{
		video_size_t new_size; 
		if (ioctl(vdec, VIDEO_GET_SIZE, &new_size) < 0)
			perror("[movieplayer.cpp] VIDEO_GET_SIZE");
		if(size.aspect_ratio != new_size.aspect_ratio)
		{
			printf("[movieplayer.cpp] AR change detected in auto mode, adjusting display format\n");
			video_displayformat_t vdt;
			if(new_size.aspect_ratio == VIDEO_FORMAT_4_3)
				vdt = VIDEO_LETTER_BOX;
			else
				vdt = VIDEO_CENTER_CUT_OUT;
			if (ioctl(vdec, VIDEO_SET_DISPLAY_FORMAT, vdt))
				perror("[movieplayer.cpp] VIDEO_SET_DISPLAY_FORMAT");
			memcpy(&size, &new_size, sizeof(size));
		}
		last_check=time(NULL);
	}
}
#endif
