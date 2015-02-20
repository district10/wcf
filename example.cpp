/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "channel.h"
#include "pluginHost.h"
#include "kernelMidi.h"
#include "patch.h"
#include "wave.h"
#include "mixer.h"
#include "mixerHandler.h"
#include "conf.h"
#include "waveFx.h"


extern Patch       G_Patch;
extern Mixer       G_Mixer;
extern Conf        G_Conf;
#ifdef WITH_VST
extern PluginHost  G_PluginHost;
#endif


Channel::Channel(int type, int status, char side, int bufferSize)
	: bufferSize(bufferSize),
	  type      (type),
		status    (status),
		side      (side),
	  volume    (DEFAULT_VOL),
	  volume_i  (1.0f),
	  volume_d  (0.0f),
	  panLeft   (1.0f),
	  panRight  (1.0f),
	  mute_i    (false),
	  mute_s    (false),
	  mute      (false),
	  solo      (false),
	  hasActions(false),
	  recStatus (REC_STOPPED),
	  vChan     (NULL),
	  guiChannel(NULL),
	  midiIn        (true),
	  midiInKeyPress(0x0),
	  midiInKeyRel  (0x0),
	  midiInKill    (0x0),
	  midiInVolume  (0x0),
	  midiInMute    (0x0),
	  midiInSolo    (0x0)
{
	vChan = (float *) malloc(bufferSize * sizeof(float));
	if (!vChan)
		printf("[Channel] unable to alloc memory for vChan\n");
	memset(vChan, 0, bufferSize * sizeof(float));
}





Channel::~Channel() {
	status = STATUS_OFF;
	if (vChan)
		free(vChan);
}





void Channel::readPatchMidiIn(int i) {
	midiIn         = G_Patch.getMidiValue(i, "In");
	midiInKeyPress = G_Patch.getMidiValue(i, "InKeyPress");
	midiInKeyRel   = G_Patch.getMidiValue(i, "InKeyRel");
  midiInKill     = G_Patch.getMidiValue(i, "InKill");
  midiInVolume   = G_Patch.getMidiValue(i, "InVolume");
  midiInMute     = G_Patch.getMidiValue(i, "InMute");
  midiInSolo     = G_Patch.getMidiValue(i, "InSolo");
}





void Channel::writePatchMidiIn(FILE *fp, int i) {
	fprintf(fp, "chanMidiIn%d=%u\n",         i, midiIn);
	fprintf(fp, "chanMidiInKeyPress%d=%u\n", i, midiInKeyPress);
	fprintf(fp, "chanMidiInKeyRel%d=%u\n",   i, midiInKeyRel);
	fprintf(fp, "chanMidiInKill%d=%u\n",     i, midiInKill);
	fprintf(fp, "chanMidiInVolume%d=%u\n",   i, midiInVolume);
	fprintf(fp, "chanMidiInMute%d=%u\n",     i, midiInMute);
	fprintf(fp, "chanMidiInSolo%d=%u\n",     i, midiInSolo);
}





bool Channel::isPlaying() {
	return status & (STATUS_PLAY | STATUS_ENDING);
}



/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "conf.h"
#include "utils.h"


int Conf::openFileForReading() {

	char path[PATH_MAX];

#if defined(__linux__)
	snprintf(path, PATH_MAX, "%s/.giada/%s", getenv("HOME"), CONF_FILENAME);
#elif defined(_WIN32)
	snprintf(path, PATH_MAX, "%s", CONF_FILENAME);
#elif defined(__APPLE__)
	struct passwd *p = getpwuid(getuid());
	if (p == NULL) {
		puts("[Conf::openFile] unable to fetch user infos.");
		return 0;
	}
	else {
		const char *home = p->pw_dir;
		snprintf(path, PATH_MAX, "%s/Library/Application Support/Giada/giada.conf", home);
	}
#endif

	fp = fopen(path, "r");
	if (fp == NULL) {
		puts("[Conf::openFile] unable to open conf file for reading.");
		return 0;
	}
	return 1;
}





int Conf::createConfigFolder(const char *path) {

	if (gDirExists(path))
		return 1;

	puts(".giada folder not present. Updating...");

	if (gMkdir(path)) {
		puts("status: ok");
		return 1;
	}
	else {
		puts("status: error!");
		return 0;
	}
}





int Conf::openFileForWriting() {

	

#if defined(__linux__)

	char giadaPath[PATH_MAX];
	sprintf(giadaPath, "%s/.giada", getenv("HOME"));

	if (!createConfigFolder(giadaPath))
		return 0;

	char path[PATH_MAX];
	sprintf(path, "%s/%s", giadaPath, CONF_FILENAME);

#elif defined(_WIN32)

	const char *path = CONF_FILENAME;

#elif defined(__APPLE__)

	struct passwd *p = getpwuid(getuid());
	const char *home = p->pw_dir;
	char giadaPath[PATH_MAX];
	snprintf(giadaPath, PATH_MAX, "%s/Library/Application Support/Giada", home);

	if (!createConfigFolder(giadaPath))
		return 0;

	char path[PATH_MAX];
	sprintf(path, "%s/%s", giadaPath, CONF_FILENAME);

#endif

	fp = fopen(path, "w");
	if (fp == NULL)
		return 0;
	return 1;

}





void Conf::setDefault() {
	soundSystem    = DEFAULT_SOUNDSYS;
	soundDeviceOut = DEFAULT_SOUNDDEV_OUT;
	soundDeviceIn  = DEFAULT_SOUNDDEV_IN;
	samplerate     = DEFAULT_SAMPLERATE;
	buffersize     = DEFAULT_BUFSIZE;
	delayComp      = DEFAULT_DELAYCOMP;
	limitOutput    = false;
	rsmpQuality    = 0;

	midiPortIn  = DEFAULT_MIDI_PORT_IN;
	midiPortOut = DEFAULT_MIDI_PORT_OUT;
	midiSync    = MIDI_SYNC_NONE;
	midiTCfps   = 25.0f;

	midiInRewind     = 0x0;
	midiInStartStop  = 0x0;
	midiInActionRec  = 0x0;
	midiInInputRec   = 0x0;
	midiInVolumeIn   = 0x0;
	midiInVolumeOut  = 0x0;
	midiInBeatDouble = 0x0;
	midiInBeatHalf   = 0x0;
	midiInMetronome  = 0x0;

	pluginPath[0]  = '\0';
	patchPath [0]  = '\0';
	samplePath[0]  = '\0';

	recsStopOnChanHalt = false;
	chansStopOnSeqHalt = false;
	treatRecsAsLoops   = false;
	fullChanVolOnLoad  = true;

	actionEditorZoom    = 100;
	actionEditorGridOn  = false;
	actionEditorGridVal = 1;

	pianoRollY = -1;
	pianoRollH = 422;
}






int Conf::read() {

	setDefault();

	if (!openFileForReading()) {
		puts("[Conf] unreadable .conf file, using default parameters");
		return 0;
	}

	if (getValue("header") != "GIADACFG") {
		puts("[Conf] corrupted .conf file, using default parameters");
		return -1;
	}

	soundSystem = atoi(getValue("soundSystem").c_str());
	if (!soundSystem & (SYS_API_ANY)) soundSystem = DEFAULT_SOUNDSYS;

	soundDeviceOut = atoi(getValue("soundDeviceOut").c_str());
	if (soundDeviceOut < 0) soundDeviceOut = DEFAULT_SOUNDDEV_OUT;

	soundDeviceIn = atoi(getValue("soundDeviceIn").c_str());
	if (soundDeviceIn < -1) soundDeviceIn = DEFAULT_SOUNDDEV_IN;

	channelsOut = atoi(getValue("channelsOut").c_str());
	channelsIn  = atoi(getValue("channelsIn").c_str());
	if (channelsOut < 0) channelsOut = 0;
	if (channelsIn < 0)  channelsIn  = 0;

	buffersize = atoi(getValue("buffersize").c_str());
	if (buffersize < 8) buffersize = DEFAULT_BUFSIZE;

	delayComp = atoi(getValue("delayComp").c_str());
	if (delayComp < 0) delayComp = DEFAULT_DELAYCOMP;

	midiSystem  = atoi(getValue("midiSystem").c_str());
	if (midiPortOut < -1) midiPortOut = DEFAULT_MIDI_SYSTEM;

	midiPortOut = atoi(getValue("midiPortOut").c_str());
	if (midiPortOut < -1) midiPortOut = DEFAULT_MIDI_PORT_OUT;

	midiPortIn = atoi(getValue("midiPortIn").c_str());
	if (midiPortIn < -1) midiPortIn = DEFAULT_MIDI_PORT_IN;

	midiSync  = atoi(getValue("midiSync").c_str());
	midiTCfps = atof(getValue("midiTCfps").c_str());

	midiInRewind     = strtoul(getValue("midiInRewind").c_str(), NULL, 10);
  midiInStartStop  = strtoul(getValue("midiInStartStop").c_str(), NULL, 10);
  midiInActionRec  = strtoul(getValue("midiInActionRec").c_str(), NULL, 10);
  midiInInputRec   = strtoul(getValue("midiInInputRec").c_str(), NULL, 10);
  midiInMetronome  = strtoul(getValue("midiInMetronome").c_str(), NULL, 10);
  midiInVolumeIn   = strtoul(getValue("midiInVolumeIn").c_str(), NULL, 10);
  midiInVolumeOut  = strtoul(getValue("midiInVolumeOut").c_str(), NULL, 10);
  midiInBeatDouble = strtoul(getValue("midiInBeatDouble").c_str(), NULL, 10);
  midiInBeatHalf   = strtoul(getValue("midiInBeatHalf").c_str(), NULL, 10);

	browserX = atoi(getValue("browserX").c_str());
	browserY = atoi(getValue("browserY").c_str());
	browserW = atoi(getValue("browserW").c_str());
	browserH = atoi(getValue("browserH").c_str());
	if (browserX < 0) browserX = 0;
	if (browserY < 0) browserY = 0;
	if (browserW < 396) browserW = 396;
	if (browserH < 302) browserH = 302;

	actionEditorX    = atoi(getValue("actionEditorX").c_str());
	actionEditorY    = atoi(getValue("actionEditorY").c_str());
	actionEditorW    = atoi(getValue("actionEditorW").c_str());
	actionEditorH    = atoi(getValue("actionEditorH").c_str());
	actionEditorZoom = atoi(getValue("actionEditorZoom").c_str());
	actionEditorGridVal = atoi(getValue("actionEditorGridVal").c_str());
	actionEditorGridOn  = atoi(getValue("actionEditorGridOn").c_str());
	if (actionEditorX < 0)      actionEditorX = 0;
	if (actionEditorY < 0)      actionEditorY = 0;
	if (actionEditorW < 640)    actionEditorW = 640;
	if (actionEditorH < 176)    actionEditorH = 176;
	if (actionEditorZoom < 100) actionEditorZoom = 100;
	if (actionEditorGridVal < 0) actionEditorGridVal = 0;
	if (actionEditorGridOn < 0)  actionEditorGridOn = 0;

	pianoRollY = atoi(getValue("pianoRollY").c_str());
	pianoRollH = atoi(getValue("pianoRollH").c_str());
	if (pianoRollH <= 0)
		pianoRollH = 422;

	sampleEditorX    = atoi(getValue("sampleEditorX").c_str());
	sampleEditorY    = atoi(getValue("sampleEditorY").c_str());
	sampleEditorW    = atoi(getValue("sampleEditorW").c_str());
	sampleEditorH    = atoi(getValue("sampleEditorH").c_str());
	if (sampleEditorX < 0)   sampleEditorX = 0;
	if (sampleEditorY < 0)   sampleEditorY = 0;
	if (sampleEditorW < 500) sampleEditorW = 500;
	if (sampleEditorH < 292) sampleEditorH = 292;

	configX = atoi(getValue("configX").c_str());
	configY = atoi(getValue("configY").c_str());
	if (configX < 0) configX = 0;
	if (configY < 0) configY = 0;

	pluginListX = atoi(getValue("pluginListX").c_str());
	pluginListY = atoi(getValue("pluginListY").c_str());
	if (pluginListX < 0) pluginListX = 0;
	if (pluginListY < 0) pluginListY = 0;

	bpmX = atoi(getValue("bpmX").c_str());
	bpmY = atoi(getValue("bpmY").c_str());
	if (bpmX < 0) bpmX = 0;
	if (bpmY < 0) bpmY = 0;

	beatsX = atoi(getValue("beatsX").c_str());
	beatsY = atoi(getValue("beatsY").c_str());
	if (beatsX < 0) beatsX = 0;
	if (beatsY < 0) beatsY = 0;

	aboutX = atoi(getValue("aboutX").c_str());
	aboutY = atoi(getValue("aboutY").c_str());
	if (aboutX < 0) aboutX = 0;
	if (aboutY < 0) aboutY = 0;

	samplerate = atoi(getValue("samplerate").c_str());
	if (samplerate < 8000) samplerate = DEFAULT_SAMPLERATE;

	limitOutput = atoi(getValue("limitOutput").c_str());
	rsmpQuality = atoi(getValue("rsmpQuality").c_str());

	std::string p = getValue("pluginPath");
	strncpy(pluginPath, p.c_str(), p.size());
	pluginPath[p.size()] = '\0';	

	p = getValue("patchPath");
	strncpy(patchPath, p.c_str(), p.size());
	patchPath[p.size()] = '\0';	

	p = getValue("samplePath");
	strncpy(samplePath, p.c_str(), p.size());
	samplePath[p.size()] = '\0';	

	recsStopOnChanHalt = atoi(getValue("recsStopOnChanHalt").c_str());
	chansStopOnSeqHalt = atoi(getValue("chansStopOnSeqHalt").c_str());
	treatRecsAsLoops   = atoi(getValue("treatRecsAsLoops").c_str());
	fullChanVolOnLoad  = atoi(getValue("fullChanVolOnLoad").c_str());

	close();
	return 1;
}





int Conf::write() {
	if (!openFileForWriting())
		return 0;

	fprintf(fp, "# --- Giada configuration file --- \n");
	fprintf(fp, "header=GIADACFG\n");
	fprintf(fp, "version=%s\n", VERSIONE);

	fprintf(fp, "soundSystem=%d\n",    soundSystem);
	fprintf(fp, "soundDeviceOut=%d\n", soundDeviceOut);
	fprintf(fp, "soundDeviceIn=%d\n",  soundDeviceIn);
	fprintf(fp, "channelsOut=%d\n",    channelsOut);
	fprintf(fp, "channelsIn=%d\n",     channelsIn);
	fprintf(fp, "buffersize=%d\n",     buffersize);
	fprintf(fp, "delayComp=%d\n",      delayComp);
	fprintf(fp, "samplerate=%d\n",     samplerate);
	fprintf(fp, "limitOutput=%d\n",    limitOutput);
	fprintf(fp, "rsmpQuality=%d\n",    rsmpQuality);

	fprintf(fp, "midiSystem=%d\n",  midiSystem);
	fprintf(fp, "midiPortOut=%d\n", midiPortOut);
	fprintf(fp, "midiPortIn=%d\n",  midiPortIn);
	fprintf(fp, "midiSync=%d\n",    midiSync);
	fprintf(fp, "midiTCfps=%f\n",   midiTCfps);

	fprintf(fp, "midiInRewind=%u\n",     midiInRewind);
	fprintf(fp, "midiInStartStop=%u\n",  midiInStartStop);
	fprintf(fp, "midiInActionRec=%u\n",  midiInActionRec);
	fprintf(fp, "midiInInputRec=%u\n",   midiInInputRec);
	fprintf(fp, "midiInMetronome=%u\n",  midiInMetronome);
	fprintf(fp, "midiInVolumeIn=%u\n",   midiInVolumeIn);
	fprintf(fp, "midiInVolumeOut=%u\n",  midiInVolumeOut);
	fprintf(fp, "midiInBeatDouble=%u\n", midiInBeatDouble);
	fprintf(fp, "midiInBeatHalf=%u\n",   midiInBeatHalf);

	fprintf(fp, "pluginPath=%s\n", pluginPath);
	fprintf(fp, "patchPath=%s\n",  patchPath);
	fprintf(fp, "samplePath=%s\n", samplePath);

	fprintf(fp, "browserX=%d\n", browserX);
	fprintf(fp, "browserY=%d\n", browserY);
	fprintf(fp, "browserW=%d\n", browserW);
	fprintf(fp, "browserH=%d\n", browserH);

	fprintf(fp, "actionEditorX=%d\n",       actionEditorX);
	fprintf(fp, "actionEditorY=%d\n",       actionEditorY);
	fprintf(fp, "actionEditorW=%d\n",       actionEditorW);
	fprintf(fp, "actionEditorH=%d\n",       actionEditorH);
	fprintf(fp, "actionEditorZoom=%d\n",    actionEditorZoom);
	fprintf(fp, "actionEditorGridOn=%d\n",  actionEditorGridOn);
	fprintf(fp, "actionEditorGridVal=%d\n", actionEditorGridVal);

	fprintf(fp, "pianoRollY=%d\n", pianoRollY);
	fprintf(fp, "pianoRollH=%d\n", pianoRollH);

	fprintf(fp, "sampleEditorX=%d\n", sampleEditorX);
	fprintf(fp, "sampleEditorY=%d\n", sampleEditorY);
	fprintf(fp, "sampleEditorW=%d\n", sampleEditorW);
	fprintf(fp, "sampleEditorH=%d\n", sampleEditorH);

	fprintf(fp, "configX=%d\n", configX);
	fprintf(fp, "configY=%d\n", configY);

	fprintf(fp, "pluginListX=%d\n", pluginListX);
	fprintf(fp, "pluginListY=%d\n", pluginListY);

	fprintf(fp, "bpmX=%d\n", bpmX);
	fprintf(fp, "bpmY=%d\n", bpmY);

	fprintf(fp, "beatsX=%d\n", beatsX);
	fprintf(fp, "beatsY=%d\n", beatsY);

	fprintf(fp, "aboutX=%d\n", aboutX);
	fprintf(fp, "aboutY=%d\n", aboutY);

	fprintf(fp, "recsStopOnChanHalt=%d\n", recsStopOnChanHalt);
	fprintf(fp, "chansStopOnSeqHalt=%d\n", chansStopOnSeqHalt);
	fprintf(fp, "treatRecsAsLoops=%d\n",   treatRecsAsLoops);
	fprintf(fp, "fullChanVolOnLoad=%d\n",  fullChanVolOnLoad);

	close();
	return 1;
}






void Conf::close() {
	if (fp != NULL)
		fclose(fp);
}





void Conf::setPath(char *path, const char *p) {
	path[0] = '\0';
	strncpy(path, p, strlen(p));
	path[strlen(p)] = '\0';
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "dataStorage.h"


std::string DataStorage::getValue(const char *in) {

	

	fseek(fp, 0L, SEEK_SET);
	std::string out = "";

	while (!feof(fp)) {

		char buffer[MAX_LINE_LEN];
		if (fgets(buffer, MAX_LINE_LEN, fp) == NULL) {
			printf("[PATCH] get_value error (key=%s)\n", in);
			return "";
		}

		if (buffer[0] == '#')
			continue;

		unsigned len = strlen(in);
		if (strncmp(buffer, in, len) == 0) {

			for (unsigned i=len+1; i<MAX_LINE_LEN; i++) {
				if (buffer[i] == '\0' || buffer[i] == '\n' || buffer[i] == '\r')
					break;
				out += buffer[i];
			}

			break; 
		}
	}
	return out;
}





/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "gd_about.h"
#include "conf.h"
#include "kernelAudio.h"
#include "ge_mixed.h"
#include "graphics.h"
#include "gui_utils.h"


extern Conf G_Conf;


gdAbout::gdAbout()
#ifdef WITH_VST
: gWindow(340, 405, "About Giada") {
#else
: gWindow(340, 320, "About Giada") {
#endif

	if (G_Conf.aboutX)
		resize(G_Conf.aboutX, G_Conf.aboutY, w(), h());

	set_modal();

	logo  = new gBox(8, 10, 324, 86);
	text  = new gBox(8, 120, 324, 145);
	close = new gClick(252, h()-28, 80, 20, "Close");
#ifdef WITH_VST
	vstLogo = new gBox(8, 265, 324, 50);
	vstText = new gBox(8, 315, 324, 46);
#endif
	end();

	logo->image(new Fl_Pixmap(giada_logo_xpm));
	text->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_TOP);

	char message[512];
	sprintf(
	  message,
	  "Version " VERSIONE " (" __DATE__ ")\n\n"
		"Developed by Monocasual\n"
		"Based on FLTK (%d.%d.%d), RtAudio (%s),\n"
		"RtMidi, libsamplerate and libsndfile\n\n"
		"Released under the terms of the GNU General\n"
		"Public License (GPL v3)\n\n"
		"News, infos, contacts and documentation:\n"
		"www.giadamusic.com",
		FL_MAJOR_VERSION, FL_MINOR_VERSION, FL_PATCH_VERSION, kernelAudio::getRtAudioVersion().c_str());

	int tw = 0;
	int th = 0;
	fl_measure(message, tw, th);
	text->copy_label(message);
	text->size(text->w(), th);

#ifdef WITH_VST
	vstLogo->image(new Fl_Pixmap(vstLogo_xpm));
	vstLogo->position(vstLogo->x(), text->y()+text->h()+8);
	vstText->label(
		"VST Plug-In Technology by Steinberg\n"
		"VST is a trademark of Steinberg\nMedia Technologies GmbH"
	);
	vstText->position(vstText->x(), vstLogo->y()+vstLogo->h());

#endif

	close->callback(cb_close, (void*)this);
	gu_setFavicon(this);
	setId(WID_ABOUT);
	show();
}





gdAbout::~gdAbout() {
	G_Conf.aboutX = x();
	G_Conf.aboutY = y();
}





void gdAbout::cb_close(Fl_Widget *w, void *p) { ((gdAbout*)p)->__cb_close(); }





void gdAbout::__cb_close() {
	do_callback();
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include <math.h>
#include "gd_actionEditor.h"
#include "ge_actionChannel.h"
#include "ge_muteChannel.h"
#include "ge_envelopeChannel.h"
#include "ge_pianoRoll.h"
#include "gui_utils.h"
#include "mixer.h"
#include "recorder.h"
#include "conf.h"
#include "ge_mixed.h"
#include "channel.h"
#include "sampleChannel.h"


extern Mixer G_Mixer;
extern Conf	 G_Conf;


gdActionEditor::gdActionEditor(Channel *chan)
	:	gWindow(640, 284),
		chan   (chan),
		zoom   (100),
		coverX (0)
{
	if (G_Conf.actionEditorW) {
		resize(G_Conf.actionEditorX, G_Conf.actionEditorY, G_Conf.actionEditorW, G_Conf.actionEditorH);
		zoom = G_Conf.actionEditorZoom;
	}

	totalWidth = (int) ceilf(G_Mixer.framesInSequencer / (float) zoom);

	

	Fl_Group *upperArea = new Fl_Group(8, 8, w()-16, 20);

	upperArea->begin();

	if (chan->type == CHANNEL_SAMPLE) {
	  actionType = new gChoice(8, 8, 80, 20);
	  gridTool   = new gGridTool(actionType->x()+actionType->w()+4, 8, this);
		actionType->add("key press");
		actionType->add("key release");
		actionType->add("kill chan");
		actionType->value(0);

		SampleChannel *ch = (SampleChannel*) chan;
		if (ch->mode == SINGLE_PRESS || ch->mode & LOOP_ANY)
		actionType->deactivate();
	}
	else {
		gridTool = new gGridTool(8, 8, this);
	}

		gBox *b1   = new gBox(gridTool->x()+gridTool->w()+4, 8, 300, 20);    
		zoomIn     = new gClick(w()-8-40-4, 8, 20, 20, "+");
		zoomOut    = new gClick(w()-8-20,   8, 20, 20, "-");
	upperArea->end();
	upperArea->resizable(b1);

	zoomIn->callback(cb_zoomIn, (void*)this);
	zoomOut->callback(cb_zoomOut, (void*)this);

	

	scroller = new gScroll(8, 36, w()-16, h()-44);

	if (chan->type == CHANNEL_SAMPLE) {

		SampleChannel *ch = (SampleChannel*) chan;

		ac = new gActionChannel     (scroller->x(), upperArea->y()+upperArea->h()+8, this, ch);
		mc = new gMuteChannel       (scroller->x(), ac->y()+ac->h()+8, this);
		vc = new gEnvelopeChannel   (scroller->x(), mc->y()+mc->h()+8, this, ACTION_VOLUME, RANGE_FLOAT, "volume");
		scroller->add(ac);
		
		scroller->add(mc);
		
		scroller->add(vc);
		

		

		vc->fill();

		

		if (ch->mode & LOOP_ANY)
			ac->deactivate();
	}
	else {
		pr = new gPianoRollContainer(scroller->x(), upperArea->y()+upperArea->h()+8, this);
		scroller->add(pr);
		scroller->add(new gResizerBar(pr->x(), pr->y()+pr->h(), scroller->w(), 8));
	}

	end();

	

	update();
	gridTool->calc();

	gu_setFavicon(this);

	char buf[256];
	sprintf(buf, "Edit Actions in Channel %d", chan->index+1);
	label(buf);

	set_non_modal();
	size_range(640, 284);
	resizable(scroller);

	show();
}





gdActionEditor::~gdActionEditor() {
	G_Conf.actionEditorX = x();
	G_Conf.actionEditorY = y();
	G_Conf.actionEditorW = w();
	G_Conf.actionEditorH = h();
	G_Conf.actionEditorZoom = zoom;

	

}





void gdActionEditor::cb_zoomIn(Fl_Widget *w, void *p)  { ((gdActionEditor*)p)->__cb_zoomIn(); }
void gdActionEditor::cb_zoomOut(Fl_Widget *w, void *p) { ((gdActionEditor*)p)->__cb_zoomOut(); }





void gdActionEditor::__cb_zoomIn() {

	

	if (zoom <= 50)
		return;

	zoom /= 2;

	update();

	if (chan->type == CHANNEL_SAMPLE) {
		ac->size(totalWidth, ac->h());
		mc->size(totalWidth, mc->h());
		vc->size(totalWidth, vc->h());
		ac->updateActions();
		mc->updateActions();
		vc->updateActions();
	}
	else {
		pr->size(totalWidth, pr->h());
		pr->updateActions();
	}

	

	int shift = Fl::event_x() + scroller->xposition();
	scroller->scroll_to(scroller->xposition() + shift, scroller->yposition());

	

	gridTool->calc();
	scroller->redraw();
}





void gdActionEditor::__cb_zoomOut() {

	zoom *= 2;

	update();

	if (chan->type == CHANNEL_SAMPLE) {
		ac->size(totalWidth, ac->h());
		mc->size(totalWidth, mc->h());
		vc->size(totalWidth, vc->h());
		ac->updateActions();
		mc->updateActions();
		vc->updateActions();
	}
	else {
		pr->size(totalWidth, pr->h());
		pr->updateActions();
	}

	

	int shift = (Fl::event_x() + scroller->xposition()) / -2;
	if (scroller->xposition() + shift < 0)
			shift = 0;
	scroller->scroll_to(scroller->xposition() + shift, scroller->yposition());

	

	gridTool->calc();
	scroller->redraw();
}





void gdActionEditor::update() {
	totalWidth = (int) ceilf(G_Mixer.framesInSequencer / (float) zoom);
	if (totalWidth < scroller->w()) {
		totalWidth = scroller->w();
		zoom = (int) ceilf(G_Mixer.framesInSequencer / (float) totalWidth);
		scroller->scroll_to(0, scroller->yposition());
	}
}





int gdActionEditor::handle(int e) {
	int ret = Fl_Group::handle(e);
	switch (e) {
		case FL_MOUSEWHEEL: {
			Fl::event_dy() == -1 ? __cb_zoomIn() : __cb_zoomOut();
			ret = 1;
			break;
		}
	}
	return ret;
}





int gdActionEditor::getActionType() {
	if (actionType->value() == 0)
		return ACTION_KEYPRESS;
	else
	if (actionType->value() == 1)
		return ACTION_KEYREL;
	else
	if (actionType->value() == 2)
		return ACTION_KILLCHAN;
	else
		return -1;
}







gGridTool::gGridTool(int x, int y, gdActionEditor *parent)
	:	Fl_Group(x, y, 80, 20), parent(parent)
{
	gridType = new gChoice(x, y, 40, 20);
	gridType->add("1");
	gridType->add("2");
	gridType->add("3");
	gridType->add("4");
	gridType->add("6");
	gridType->add("8");
	gridType->add("16");
	gridType->add("32");
	gridType->value(0);
	gridType->callback(cb_changeType, (void*)this);

	active = new gCheck (x+44, y+4, 12, 12);

	gridType->value(G_Conf.actionEditorGridVal);
	active->value(G_Conf.actionEditorGridOn);

	end();
}





gGridTool::~gGridTool() {
	G_Conf.actionEditorGridVal = gridType->value();
	G_Conf.actionEditorGridOn  = active->value();
}





void gGridTool::cb_changeType(Fl_Widget *w, void *p)  { ((gGridTool*)p)->__cb_changeType(); }





void gGridTool::__cb_changeType() {
	calc();
	parent->redraw();
}





bool gGridTool::isOn() {
	return active->value();
}





int gGridTool::getValue() {
	switch (gridType->value()) {
		case 0:	return 1;
		case 1: return 2;
		case 2: return 3;
		case 3: return 4;
		case 4: return 6;
		case 5: return 8;
		case 6: return 16;
		case 7: return 32;
	}
	return 0;
}





void gGridTool::calc() {

	points.clear();
	frames.clear();
	bars.clear();
	beats.clear();

	

	int  j   = 0;
	int fpgc = floor(G_Mixer.framesPerBeat / getValue());  

	for (int i=1; i<parent->totalWidth; i++) {   
		int step = parent->zoom*i;
		while (j < step && j < G_Mixer.totalFrames) {
			if (j % fpgc == 0) {
				points.add(i);
				frames.add(j);
			}
			if (j % G_Mixer.framesPerBeat == 0)
				beats.add(i);
			if (j % G_Mixer.framesPerBar == 0 && i != 1)
				bars.add(i);
			if (j == G_Mixer.totalFrames-1)
				parent->coverX = i;
			j++;
		}
		j = step;
	}

	

	if (G_Mixer.beats == 32)
		parent->coverX = parent->totalWidth;
}





int gGridTool::getSnapPoint(int v) {

	if (v == 0) return 0;

	for (int i=0; i<(int)points.size; i++) {

		if (i == (int) points.size-1)
			return points.at(i);

		int gp  = points.at(i);
		int gpn = points.at(i+1);

		if (v >= gp && v < gpn)
			return gp;
	}
	return v;  
}





int gGridTool::getSnapFrame(int v) {

	v *= parent->zoom;  

	for (int i=0; i<(int)frames.size; i++) {

		if (i == (int) frames.size-1)
			return frames.at(i);

		int gf  = frames.at(i);     
		int gfn = frames.at(i+1);   

		if (v >= gf && v < gfn) {

			

			if ((gfn - v) < (v - gf))
				return gfn;
			else
				return gf;
		}
	}
	return v;  
}





int gGridTool::getCellSize() {
	return (parent->coverX - parent->ac->x()) / G_Mixer.beats / getValue();
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "gd_beatsInput.h"
#include "patch.h"
#include "gd_mainWindow.h"
#include "gui_utils.h"
#include "glue.h"
#include "mixer.h"


extern Mixer				 G_Mixer;
extern Conf          G_Conf;
extern gdMainWindow *mainWin;


gdBeatsInput::gdBeatsInput()
	: gWindow(164, 60, "Beats")
{
	if (G_Conf.beatsX)
		resize(G_Conf.beatsX, G_Conf.beatsY, w(), h());

	set_modal();

	beats   = new gInput(8,  8,  35, 20);
	bars    = new gInput(47, 8,  35, 20);
	ok 		  = new gClick(86, 8,  70, 20, "Ok");
	exp 	  = new gCheck(8,  40, 12, 12, "resize recordings");
	end();

	char buf_bars[3]; sprintf(buf_bars, "%d", G_Mixer.bars);
	char buf_beats[3]; sprintf(buf_beats, "%d", G_Mixer.beats);
	beats->maximum_size(2);
	beats->value(buf_beats);
	beats->type(FL_INT_INPUT);
	bars->maximum_size(2);
	bars->value(buf_bars);
	bars->type(FL_INT_INPUT);
	ok->shortcut(FL_Enter);
	ok->callback(cb_update_batt, (void*)this);
	exp->value(1); 

	gu_setFavicon(this);
	setId(WID_BEATS);
	show();
}





gdBeatsInput::~gdBeatsInput() {
	G_Conf.beatsX = x();
	G_Conf.beatsY = y();
}





void gdBeatsInput::cb_update_batt(Fl_Widget *w, void *p) { ((gdBeatsInput*)p)->__cb_update_batt(); }





void gdBeatsInput::__cb_update_batt() {
	if (!strcmp(beats->value(), "") || !strcmp(bars->value(), ""))
		return;
	glue_setBeats(atoi(beats->value()), atoi(bars->value()), exp->value());
	do_callback();
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "gd_bpmInput.h"
#include "gd_mainWindow.h"
#include "conf.h"
#include "ge_mixed.h"
#include "mixer.h"
#include "gui_utils.h"
#include "glue.h"


extern Mixer     		 G_Mixer;
extern Conf          G_Conf;
extern gdMainWindow *mainWin;


gdBpmInput::gdBpmInput()
: gWindow(144, 36, "Bpm") {

	if (G_Conf.bpmX)
		resize(G_Conf.bpmX, G_Conf.bpmY, w(), h());

	set_modal();

	input_a = new gInput(8,  8, 30, 20);
	input_b = new gInput(42, 8, 20, 20);
	ok 		  = new gClick(66, 8, 70, 20, "Ok");
	end();

	char   a[4];
	snprintf(a, 4, "%d", (int) G_Mixer.bpm);
	char   b[2];
	for (unsigned i=0; i<strlen(mainWin->bpm->label()); i++)	
		if (mainWin->bpm->label()[i] == '.') {
			snprintf(b, 2, "%c", mainWin->bpm->label()[i+1]);
			break;
		}

	input_a->maximum_size(3);
	input_a->type(FL_INT_INPUT);
	input_a->value(a);
	input_b->maximum_size(1);
	input_b->type(FL_INT_INPUT);
	input_b->value(b);

	ok->shortcut(FL_Enter);
	ok->callback(cb_update_bpm, (void*)this);

	gu_setFavicon(this);
	setId(WID_BPM);
	show();
}





gdBpmInput::~gdBpmInput() {
	G_Conf.bpmX = x();
	G_Conf.bpmY = y();
}





void gdBpmInput::cb_update_bpm(Fl_Widget *w, void *p) { ((gdBpmInput*)p)->__cb_update_bpm(); }





void gdBpmInput::__cb_update_bpm() {
	if (strcmp(input_a->value(), "") == 0)
		return;
	glue_setBpm(input_a->value(), input_b->value());
	do_callback();
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "gd_browser.h"
#include "ge_browser.h"
#include "mixer.h"
#include "gd_pluginList.h"
#include "gd_mainWindow.h"
#include "graphics.h"
#include "gd_warnings.h"
#include "wave.h"
#include "glue.h"
#include "pluginHost.h"
#include "channel.h"
#include "sampleChannel.h"


extern Patch         G_Patch;
extern Conf	         G_Conf;
extern Mixer         G_Mixer;
#ifdef WITH_VST
extern PluginHost    G_PluginHost;
#endif
extern gdMainWindow	*mainWin;


gdBrowser::gdBrowser(const char *title, const char *initPath, Channel *ch, int type, int stackType)
	:	gWindow  (396, 302, title),
		ch       (ch),
		type     (type),
		stackType(stackType)
{
	set_non_modal();

	browser = new gBrowser(8, 36, 380, 230);
	Fl_Group *group_btn = new Fl_Group(8, 274, 380, 20);
		gBox *b = new gBox(8, 274, 204, 20); 					        
		ok  	  = new gClick(308, 274, 80, 20);
		cancel  = new gClick(220, 274, 80, 20, "Cancel");
		status  = new gProgress(8, 274, 204, 20);
		status->minimum(0);
		status->maximum(1);
		status->hide();   
	group_btn->resizable(b);
	group_btn->end();

	Fl_Group *group_upd = new Fl_Group(8, 8, 380, 25);
		if (type == BROWSER_SAVE_PATCH || type == BROWSER_SAVE_SAMPLE || type == BROWSER_SAVE_PROJECT)  
			name = new gInput(208, 8, 152, 20);
		if (type == BROWSER_SAVE_PATCH || type == BROWSER_SAVE_SAMPLE || type == BROWSER_SAVE_PROJECT)  
			where = new gInput(8, 8, 192, 20);
		else
			where = new gInput(8, 8, 352, 20);
		updir	= new gClick(368, 8, 20, 20, "", updirOff_xpm, updirOn_xpm);
	group_upd->resizable(where);
	group_upd->end();

	end();

	resizable(browser);
	size_range(w(), h(), 0, 0);

	where->readonly(true);
	where->cursor_color(COLOR_BG_DARK);

	if (type == BROWSER_SAVE_PATCH || type == BROWSER_SAVE_SAMPLE || type == BROWSER_SAVE_PROJECT)  
		ok->label("Save");
	else
		ok->label("Load");

	if (type == BROWSER_LOAD_PATCH)
		ok->callback(cb_load_patch, (void*)this);
	else
	if (type == BROWSER_LOAD_SAMPLE)
		ok->callback(cb_load_sample, (void*)this);
	else
	if (type == BROWSER_SAVE_PATCH) {
		ok->callback(cb_save_patch, (void*)this);
		name->value(G_Patch.name[0] == '\0' ? "my_patch.gptc" : G_Patch.name);
		name->maximum_size(MAX_PATCHNAME_LEN+5); 
	}
	else
	if (type == BROWSER_SAVE_SAMPLE) {
		ok->callback(cb_save_sample, (void*)this);
		name->value(((SampleChannel*)ch)->wave->name.c_str());
	}
	else
	if (type == BROWSER_SAVE_PROJECT) {
		ok->callback(cb_save_project, (void*)this);
		name->value(gStripExt(G_Patch.name).c_str());
	}
#ifdef WITH_VST
	else
	if (type == BROWSER_LOAD_PLUGIN) {
		ok->callback(cb_loadPlugin, (void*)this);
	}
#endif

	ok->shortcut(FL_Enter);

	updir->callback(cb_up, (void*)this);
	cancel->callback(cb_close, (void*)this);
	browser->callback(cb_down, this);
	browser->path_obj = where;
	browser->init(initPath);

	if (G_Conf.browserW)
		resize(G_Conf.browserX, G_Conf.browserY, G_Conf.browserW, G_Conf.browserH);

	gu_setFavicon(this);
	show();
}





gdBrowser::~gdBrowser() {
	G_Conf.browserX = x();
	G_Conf.browserY = y();
	G_Conf.browserW = w();
	G_Conf.browserH = h();
}





void gdBrowser::cb_load_patch  (Fl_Widget *v, void *p)  { ((gdBrowser*)p)->__cb_load_patch();  }
void gdBrowser::cb_load_sample (Fl_Widget *v, void *p)  { ((gdBrowser*)p)->__cb_load_sample(); }
void gdBrowser::cb_save_sample (Fl_Widget *v, void *p)  { ((gdBrowser*)p)->__cb_save_sample(); }
void gdBrowser::cb_save_patch  (Fl_Widget *v, void *p)  { ((gdBrowser*)p)->__cb_save_patch(); }
void gdBrowser::cb_save_project(Fl_Widget *v, void *p)  { ((gdBrowser*)p)->__cb_save_project(); }
void gdBrowser::cb_down        (Fl_Widget *v, void *p)  { ((gdBrowser*)p)->__cb_down(); }
void gdBrowser::cb_up          (Fl_Widget *v, void *p)  { ((gdBrowser*)p)->__cb_up(); }
void gdBrowser::cb_close       (Fl_Widget *v, void *p)  { ((gdBrowser*)p)->__cb_close(); }
#ifdef WITH_VST
void gdBrowser::cb_loadPlugin  (Fl_Widget *v, void *p)  { ((gdBrowser*)p)->__cb_loadPlugin(); }
#endif





void gdBrowser::__cb_load_patch() {

	if (browser->text(browser->value()) == NULL)
		return;

	

	std::string patchFile = browser->get_selected_item();;
	bool        isProject;

	if (gIsProject(browser->get_selected_item())) {
		std::string patchName = gGetProjectName(browser->get_selected_item());
#if defined(__linux__) || defined(__APPLE__)
		patchFile = patchFile+"/"+patchName+".gptc";
#elif defined(_WIN32)
		patchFile = patchFile+"\\"+patchName+".gptc";
#endif
		isProject = true;
	}
	else
		isProject = false;

	int res = glue_loadPatch(patchFile.c_str(),	browser->path_obj->value(),	status, isProject);

	if (res == PATCH_UNREADABLE) {
		status->hide();
		if (isProject)
			gdAlert("This project is unreadable.");
		else
			gdAlert("This patch is unreadable.");
	}
	else if (res == PATCH_INVALID) {
		status->hide();
		if (isProject)
			gdAlert("This project is not valid.");
		else
			gdAlert("This patch is not valid.");
	}
	else
		do_callback();
}





void gdBrowser::__cb_save_sample() {

	if (strcmp(name->value(), "") == 0) {    
		gdAlert("Please choose a file name.");
		return;
	}

	

	std::string filename = gStripExt(name->value());
	char fullpath[PATH_MAX];
	sprintf(fullpath, "%s/%s.wav", where->value(), filename.c_str());

	if (gFileExists(fullpath))
		if (!gdConfirmWin("Warning", "File exists: overwrite?"))
			return;

	if (((SampleChannel*)ch)->save(fullpath))
		do_callback();
	else
		gdAlert("Unable to save this sample!");
}





void gdBrowser::__cb_load_sample() {
	if (browser->text(browser->value()) == NULL)
		return;

	int res = glue_loadChannel((SampleChannel*) ch, browser->get_selected_item(), browser->path_obj->value());

	if (res == SAMPLE_LOADED_OK) {
		do_callback();
		mainWin->delSubWindow(WID_SAMPLE_EDITOR); 
	}
	else if (res == SAMPLE_NOT_VALID)
		gdAlert("This is not a valid WAVE file.");
	else if (res == SAMPLE_MULTICHANNEL)
		gdAlert("Multichannel samples not supported.");
	else if (res == SAMPLE_WRONG_BIT)
		gdAlert("This sample has an\nunsupported bit-depth (> 32 bit).");
	else if (res == SAMPLE_WRONG_ENDIAN)
		gdAlert("This sample has a wrong\nbyte order (not little-endian).");
	else if (res == SAMPLE_WRONG_FORMAT)
		gdAlert("This sample is encoded in\nan unsupported audio format.");
	else if (res == SAMPLE_READ_ERROR)
		gdAlert("Unable to read this sample.");
	else if (res == SAMPLE_PATH_TOO_LONG)
		gdAlert("File path too long.");
	else
		gdAlert("Unknown error.");
}





void gdBrowser::__cb_down() {
	const char *path = browser->get_selected_item();
	if (!path)  
		return;
	if (!gIsDir(path)) {

		

		if (type == BROWSER_SAVE_PATCH || type == BROWSER_SAVE_SAMPLE || type == BROWSER_SAVE_PROJECT) {
			if (gIsProject(path)) {
				std::string tmp = browser->text(browser->value());
				tmp.erase(0, 4);
				name->value(tmp.c_str());
			}
			else
				name->value(browser->text(browser->value()));
		}
		return;
	}
	browser->clear();
	browser->down_dir(path);
	browser->sort();
}





void gdBrowser::__cb_up() {
	browser->clear();
	browser->up_dir();
	browser->sort();
}





void gdBrowser::__cb_save_patch() {

	if (strcmp(name->value(), "") == 0) {  
		gdAlert("Please choose a file name.");
		return;
	}

	

	char ext[6] = ".gptc";
	if (strstr(name->value(), ".gptc") != NULL)
		ext[0] = '\0';

	char fullpath[PATH_MAX];
	sprintf(fullpath, "%s/%s%s", where->value(), name->value(), ext);
	if (gFileExists(fullpath))
		if (!gdConfirmWin("Warning", "File exists: overwrite?"))
			return;

	if (glue_savePatch(fullpath, name->value(), false)) 
		do_callback();
	else
		gdAlert("Unable to save the patch!");
}





void gdBrowser::__cb_save_project() {

	if (strcmp(name->value(), "") == 0) {    
		gdAlert("Please choose a project name.");
		return;
	}

	

	char ext[6] = ".gprj";
	if (strstr(name->value(), ".gprj") != NULL)
		ext[0] = '\0';

	char fullpath[PATH_MAX];
#if defined(_WIN32)
	sprintf(fullpath, "%s\\%s%s", where->value(), name->value(), ext);
#else
	sprintf(fullpath, "%s/%s%s", where->value(), name->value(), ext);
#endif

	if (gIsProject(fullpath) && !gdConfirmWin("Warning", "Project exists: overwrite?"))
		return;

	if (glue_saveProject(fullpath, name->value()))
		do_callback();
	else
		gdAlert("Unable to save the project!");
}





#ifdef WITH_VST
void gdBrowser::__cb_loadPlugin() {

	if (browser->text(browser->value()) == NULL)
		return;

	int res = G_PluginHost.addPlugin(browser->get_selected_item(), stackType, ch);

	

	G_Conf.setPath(G_Conf.pluginPath, where->value());

	if (res)
		do_callback();
	else
		gdAlert("Unable to load the selected plugin!");
}
#endif





void gdBrowser::__cb_close() {
	do_callback();
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "gd_config.h"
#include "gd_keyGrabber.h"
#include "gd_devInfo.h"
#include "ge_mixed.h"
#include "conf.h"
#include "gui_utils.h"
#include "patch.h"
#include "kernelAudio.h"
#include "kernelMidi.h"


extern Patch G_Patch;
extern Conf	 G_Conf;
extern bool  G_audio_status;


gTabAudio::gTabAudio(int X, int Y, int W, int H)
	: Fl_Group(X, Y, W, H, "Sound System")
{
	begin();
	soundsys    = new gChoice(x()+92,  y()+9,  253, 20, "System");
	buffersize  = new gChoice(x()+92,  y()+37, 55,  20, "Buffer size");
	samplerate  = new gChoice(x()+290, y()+37, 55,  20, "Sample rate");
	sounddevOut = new gChoice(x()+92,  y()+65, 225, 20, "Output device");
	devOutInfo  = new gClick (x()+325, y()+65, 20,  20, "?");
	channelsOut = new gChoice(x()+92,  y()+93, 55,  20, "Output channels");
	limitOutput = new gCheck (x()+155, y()+97, 55,  20, "Limit output");
	sounddevIn  = new gChoice(x()+92,  y()+121, 225, 20, "Input device");
	devInInfo   = new gClick (x()+325, y()+121, 20,  20, "?");
	channelsIn  = new gChoice(x()+92,  y()+149, 55,  20, "Input channels");
	delayComp   = new gInput (x()+290, y()+149, 55,  20, "Rec delay comp.");
	rsmpQuality = new gChoice(x()+92, y()+177, 253, 20, "Resampling");
                new gBox(x(), rsmpQuality->y()+rsmpQuality->h()+8, w(), 92, "Restart Giada for the changes to take effect.");
	end();
	labelsize(11);

#if defined(__linux__)

	if (kernelAudio::hasAPI(RtAudio::LINUX_ALSA))
		soundsys->add("ALSA");
	if (kernelAudio::hasAPI(RtAudio::UNIX_JACK))
		soundsys->add("Jack");
	if (kernelAudio::hasAPI(RtAudio::LINUX_PULSE))
		soundsys->add("PulseAudio");

	switch (G_Conf.soundSystem) {
		case SYS_API_ALSA:
			soundsys->show("ALSA");
			break;
		case SYS_API_JACK:
			soundsys->show("Jack");
			buffersize->deactivate();
			samplerate->deactivate();
			break;
		case SYS_API_PULSE:
			soundsys->show("PulseAudio");
			break;
	}
	soundsysInitValue = soundsys->value();

#elif defined(_WIN32)

	if (kernelAudio::hasAPI(RtAudio::WINDOWS_DS))
		soundsys->add("DirectSound");
	if (kernelAudio::hasAPI(RtAudio::WINDOWS_ASIO))
		soundsys->add("ASIO");
	soundsys->show(G_Conf.soundSystem == SYS_API_DS ? "DirectSound" : "ASIO");
	soundsysInitValue = soundsys->value();

#elif defined (__APPLE__)

	if (kernelAudio::hasAPI(RtAudio::MACOSX_CORE))
		soundsys->add("CoreAudio");
	soundsys->show("CoreAudio");
	soundsysInitValue = soundsys->value();

#endif

	sounddevIn->callback(cb_fetchInChans, this);
	sounddevOut->callback(cb_fetchOutChans, this);

	devOutInfo->callback(cb_showOutputInfo, this);
	devInInfo->callback(cb_showInputInfo, this);

	fetchSoundDevs();

	fetchOutChans(sounddevOut->value());
	fetchInChans(sounddevIn->value());

	buffersize->add("8");
	buffersize->add("16");
	buffersize->add("32");
	buffersize->add("64");
	buffersize->add("128");
	buffersize->add("256");
	buffersize->add("512");
	buffersize->add("1024");
	buffersize->add("2048");
	buffersize->add("4096");

	char buf[8];
	sprintf(buf, "%d", G_Conf.buffersize);
	buffersize->show(buf);

	

	int nfreq = kernelAudio::getTotalFreqs(sounddevOut->value());
	for (int i=0; i<nfreq; i++) {
		char buf[16];
		int  freq = kernelAudio::getFreq(sounddevOut->value(), i);
		sprintf(buf, "%d", freq);
		samplerate->add(buf);
		if (freq == G_Conf.samplerate)
			samplerate->value(i);
	}

	rsmpQuality->add("Sinc best quality (very slow)");
	rsmpQuality->add("Sinc medium quality (slow)");
	rsmpQuality->add("Sinc basic quality (medium)");
	rsmpQuality->add("Zero Order Hold (fast)");
	rsmpQuality->add("Linear (very fast)");
	rsmpQuality->value(G_Conf.rsmpQuality);

	buf[0] = '\0';
	sprintf(buf, "%d", G_Conf.delayComp);
	delayComp->value(buf);
	delayComp->type(FL_INT_INPUT);
	delayComp->maximum_size(5);

	limitOutput->value(G_Conf.limitOutput);
	soundsys->callback(cb_deactivate_sounddev, (void*)this);
}





void gTabAudio::cb_deactivate_sounddev(Fl_Widget *w, void *p) { ((gTabAudio*)p)->__cb_deactivate_sounddev(); }
void gTabAudio::cb_fetchInChans(Fl_Widget *w, void *p)        { ((gTabAudio*)p)->__cb_fetchInChans(); }
void gTabAudio::cb_fetchOutChans(Fl_Widget *w, void *p)       { ((gTabAudio*)p)->__cb_fetchOutChans(); }
void gTabAudio::cb_showInputInfo(Fl_Widget *w, void *p)       { ((gTabAudio*)p)->__cb_showInputInfo(); }
void gTabAudio::cb_showOutputInfo(Fl_Widget *w, void *p)      { ((gTabAudio*)p)->__cb_showOutputInfo(); }





void gTabAudio::__cb_fetchInChans() {
	fetchInChans(sounddevIn->value());
	channelsIn->value(0);
}





void gTabAudio::__cb_fetchOutChans() {
	fetchOutChans(sounddevOut->value());
	channelsOut->value(0);
}




void gTabAudio::__cb_showInputInfo() {
	unsigned dev = kernelAudio::getDeviceByName(sounddevIn->text(sounddevIn->value()));
	new gdDevInfo(dev);
}





void gTabAudio::__cb_showOutputInfo() {
	unsigned dev = kernelAudio::getDeviceByName(sounddevOut->text(sounddevOut->value()));
	new gdDevInfo(dev);
}





void gTabAudio::__cb_deactivate_sounddev() {

	

	if (soundsysInitValue == soundsys->value()) {
		sounddevOut->clear();
		sounddevIn->clear();

		fetchSoundDevs();

		

		fetchOutChans(sounddevOut->value());
		sounddevOut->activate();
		channelsOut->activate();

		

		fetchInChans(sounddevIn->value());
		sounddevIn->activate();
	}
	else {
		sounddevOut->deactivate();
		sounddevOut->clear();
		sounddevOut->add("-- restart to fetch device(s) --");
		sounddevOut->value(0);
		channelsOut->deactivate();
		devOutInfo->deactivate();

		sounddevIn->deactivate();
		sounddevIn->clear();
		sounddevIn->add("-- restart to fetch device(s) --");
		sounddevIn->value(0);
		channelsIn->deactivate();
		devInInfo->deactivate();
	}
}




void gTabAudio::fetchInChans(int menuItem) {

	

	if (menuItem == 0) {
		devInInfo ->deactivate();
		channelsIn->deactivate();
		delayComp ->deactivate();
		return;
	}

	devInInfo ->activate();
	channelsIn->activate();
	delayComp ->activate();

	channelsIn->clear();

	unsigned dev = kernelAudio::getDeviceByName(sounddevIn->text(sounddevIn->value()));
	unsigned chs = kernelAudio::getMaxInChans(dev);

	if (chs == 0) {
		channelsIn->add("none");
		channelsIn->value(0);
		return;
	}
	for (unsigned i=0; i<chs; i+=2) {
		char str[16];
		sprintf(str, "%d-%d", (i+1), (i+2));
		channelsIn->add(str);
	}
	channelsIn->value(G_Conf.channelsIn);
}





void gTabAudio::fetchOutChans(int menuItem) {

	channelsOut->clear();

	unsigned dev = kernelAudio::getDeviceByName(sounddevOut->text(sounddevOut->value()));
	unsigned chs = kernelAudio::getMaxOutChans(dev);

	if (chs == 0) {
		channelsOut->add("none");
		channelsOut->value(0);
		return;
	}
	for (unsigned i=0; i<chs; i+=2) {
		char str[16];
		sprintf(str, "%d-%d", (i+1), (i+2));
		channelsOut->add(str);
	}
	channelsOut->value(G_Conf.channelsOut);
}





int gTabAudio::findMenuDevice(gChoice *m, int device) {

	if (device == -1)
		return 0;

	if (G_audio_status == false)
		return 0;

	for (int i=0; i<m->size(); i++) {
		if (kernelAudio::getDeviceName(device) == NULL)
			continue;
		if (m->text(i) == NULL)
			continue;
		if (strcmp(m->text(i), kernelAudio::getDeviceName(device))==0)
			return i;
	}

	return 0;
}





void gTabAudio::fetchSoundDevs() {

	if (kernelAudio::numDevs == 0) {
		sounddevOut->add("-- no devices found --");
		sounddevOut->value(0);
		sounddevIn->add("-- no devices found --");
		sounddevIn->value(0);
		devInInfo ->deactivate();
		devOutInfo->deactivate();
	}
	else {

		devInInfo ->activate();
		devOutInfo->activate();

		

		sounddevIn->add("(disabled)");

		for (unsigned i=0; i<kernelAudio::numDevs; i++) {

			

			std::string tmp = kernelAudio::getDeviceName(i);
			for (unsigned k=0; k<tmp.size(); k++)
				if (tmp[k] == '/' || tmp[k] == '|' || tmp[k] == '&' || tmp[k] == '_')
					tmp[k] = '-';

			

			if (kernelAudio::getMaxOutChans(i) > 0)
				sounddevOut->add(tmp.c_str());

			if (kernelAudio::getMaxInChans(i) > 0)
				sounddevIn->add(tmp.c_str());
		}

		

		if (sounddevOut->size() == 0) {
			sounddevOut->add("-- no devices found --");
			sounddevOut->value(0);
			devOutInfo->deactivate();
		}
		else {
			int outMenuValue = findMenuDevice(sounddevOut, G_Conf.soundDeviceOut);
			sounddevOut->value(outMenuValue);
		}

		if (sounddevIn->size() == 0) {
			sounddevIn->add("-- no devices found --");
			sounddevIn->value(0);
			devInInfo->deactivate();
		}
		else {
			int inMenuValue = findMenuDevice(sounddevIn, G_Conf.soundDeviceIn);
			sounddevIn->value(inMenuValue);
		}
	}
}





void gTabAudio::save() {

	

#ifdef __linux__
	if      (soundsys->value() == 0)	G_Conf.soundSystem = SYS_API_ALSA;
	else if (soundsys->value() == 1)	G_Conf.soundSystem = SYS_API_JACK;
	else if (soundsys->value() == 2)	G_Conf.soundSystem = SYS_API_PULSE;
#else
#ifdef _WIN32
	if 			(soundsys->value() == 0)	G_Conf.soundSystem = SYS_API_DS;
	else if (soundsys->value() == 1)  G_Conf.soundSystem = SYS_API_ASIO;
#endif
#endif

	

	G_Conf.soundDeviceOut = kernelAudio::getDeviceByName(sounddevOut->text(sounddevOut->value()));
	G_Conf.soundDeviceIn  = kernelAudio::getDeviceByName(sounddevIn->text(sounddevIn->value()));
	G_Conf.channelsOut    = channelsOut->value();
	G_Conf.channelsIn     = channelsIn->value();
	G_Conf.limitOutput    = limitOutput->value();
	G_Conf.rsmpQuality    = rsmpQuality->value();

	

	if (G_Conf.soundDeviceOut == -1)
		G_Conf.soundDeviceOut = 0;

	int bufsize = atoi(buffersize->text());
	if (bufsize % 2 != 0) bufsize++;
	if (bufsize < 8)		  bufsize = 8;
	if (bufsize > 8192)		bufsize = 8192;
	G_Conf.buffersize = bufsize;

	const Fl_Menu_Item *i = NULL;
	i = samplerate->mvalue(); 
	if (i)
		G_Conf.samplerate = atoi(i->label());

	int _delayComp = atoi(delayComp->value());
	if (_delayComp < 0) _delayComp = 0;
	G_Conf.delayComp = _delayComp;
}







gTabMidi::gTabMidi(int X, int Y, int W, int H)
	: Fl_Group(X, Y, W, H, "MIDI")
{
	begin();
	system  = new gChoice(x()+92, y()+9, 253, 20, "System");
	portOut = new gChoice(x()+92, system->y()+system->h()+8, 253, 20, "Output port");
	portIn  = new gChoice(x()+92, portOut->y()+portOut->h()+8, 253, 20, "Input port");
	sync    = new gChoice(x()+92, portIn->y()+portIn->h()+8, 253, 20, "Sync");
	new gBox(x(), sync->y()+sync->h()+8, w(), h()-100, "Restart Giada for the changes to take effect.");
	end();

	labelsize(11);

	system->callback(cb_changeSystem, (void*)this);

	fetchSystems();
	fetchOutPorts();
	fetchInPorts();

	sync->add("(disabled)");
	sync->add("MIDI Clock (master)");
	sync->add("MTC (master)");
	if      (G_Conf.midiSync == MIDI_SYNC_NONE)
		sync->value(0);
	else if (G_Conf.midiSync == MIDI_SYNC_CLOCK_M)
		sync->value(1);
	else if (G_Conf.midiSync == MIDI_SYNC_MTC_M)
		sync->value(2);

	systemInitValue = system->value();
}





void gTabMidi::fetchOutPorts() {

	if (kernelMidi::numOutPorts == 0) {
		portOut->add("-- no ports found --");
		portOut->value(0);
		portOut->deactivate();
	}
	else {

		portOut->add("(disabled)");

		for (unsigned i=0; i<kernelMidi::numOutPorts; i++) {
			char *t = (char*) kernelMidi::getOutPortName(i);
			for (int k=0; t[k] != '\0'; k++)
				if (t[k] == '/' || t[k] == '|' || t[k] == '&' || t[k] == '_')
					t[k] = '-';
			portOut->add(t);
		}

		portOut->value(G_Conf.midiPortOut+1);    
	}
}




void gTabMidi::fetchInPorts() {

	if (kernelMidi::numInPorts == 0) {
		portIn->add("-- no ports found --");
		portIn->value(0);
		portIn->deactivate();
	}
	else {

		portIn->add("(disabled)");

		for (unsigned i=0; i<kernelMidi::numInPorts; i++) {
			char *t = (char*) kernelMidi::getInPortName(i);
			for (int k=0; t[k] != '\0'; k++)
				if (t[k] == '/' || t[k] == '|' || t[k] == '&' || t[k] == '_')
					t[k] = '-';
			portIn->add(t);
		}

		portIn->value(G_Conf.midiPortIn+1);    
	}
}





void gTabMidi::save() {

	if      (!strcmp("ALSA", system->text(system->value())))
		G_Conf.midiSystem = RtMidi::LINUX_ALSA;
	else if (!strcmp("Jack", system->text(system->value())))
		G_Conf.midiSystem = RtMidi::UNIX_JACK;
	else if (!strcmp("Multimedia MIDI", system->text(system->value())))
		G_Conf.midiSystem = RtMidi::WINDOWS_MM;
	else if (!strcmp("Kernel Streaming MIDI", system->text(system->value())))
		G_Conf.midiSystem = RtMidi::WINDOWS_KS;
	else if (!strcmp("OSX Core MIDI", system->text(system->value())))
		G_Conf.midiSystem = RtMidi::MACOSX_CORE;

	G_Conf.midiPortOut = portOut->value()-1;   
	G_Conf.midiPortIn  = portIn->value()-1;    

	if      (sync->value() == 0)
		G_Conf.midiSync = MIDI_SYNC_NONE;
	else if (sync->value() == 1)
		G_Conf.midiSync = MIDI_SYNC_CLOCK_M;
	else if (sync->value() == 2)
		G_Conf.midiSync = MIDI_SYNC_MTC_M;
}





void gTabMidi::fetchSystems() {

#if defined(__linux__)

	if (kernelMidi::hasAPI(RtMidi::LINUX_ALSA))
		system->add("ALSA");
	if (kernelMidi::hasAPI(RtMidi::UNIX_JACK))
		system->add("Jack");

#elif defined(_WIN32)

	if (kernelMidi::hasAPI(RtMidi::WINDOWS_MM))
		system->add("Multimedia MIDI");
	if (kernelMidi::hasAPI(RtMidi::WINDOWS_KS))
		system->add("Kernel Streaming MIDI");

#elif defined (__APPLE__)

	system->add("OSX Core MIDI");

#endif

	switch (G_Conf.midiSystem) {
		case RtMidi::LINUX_ALSA:  system->show("ALSA"); break;
		case RtMidi::UNIX_JACK:   system->show("Jack"); break;
		case RtMidi::WINDOWS_MM:  system->show("Multimedia MIDI"); break;
		case RtMidi::WINDOWS_KS:  system->show("Kernel Streaming MIDI"); break;
		case RtMidi::MACOSX_CORE: system->show("OSX Core MIDI"); break;
		default: system->value(0); break;
	}
}





void gTabMidi::cb_changeSystem(Fl_Widget *w, void *p) { ((gTabMidi*)p)->__cb_changeSystem(); }





void gTabMidi::__cb_changeSystem() {

	

	if (systemInitValue == system->value()) {
		portOut->clear();
		fetchOutPorts();
		portOut->activate();
	}
	else {
		portOut->deactivate();
		portOut->clear();
		portOut->add("-- restart to fetch device(s) --");
		portOut->value(0);
	}

}







gTabBehaviors::gTabBehaviors(int X, int Y, int W, int H)
	: Fl_Group(X, Y, W, H, "Behaviors")
{
	begin();
	Fl_Group *radioGrp_1 = new Fl_Group(x(), y()+10, w(), 70); 
		new gBox(x(), y()+10, 70, 25, "When a channel with recorded actions is halted:", FL_ALIGN_LEFT);
		recsStopOnChanHalt_1 = new gRadio(x()+25, y()+35, 280, 20, "stop it immediately");
		recsStopOnChanHalt_0 = new gRadio(x()+25, y()+55, 280, 20, "play it until finished");
	radioGrp_1->end();

	Fl_Group *radioGrp_2 = new Fl_Group(x(), y()+70, w(), 70); 
		new gBox(x(), y()+80, 70, 25, "When the sequencer is halted:", FL_ALIGN_LEFT);
		chansStopOnSeqHalt_1 = new gRadio(x()+25, y()+105, 280, 20, "stop immediately all dynamic channels");
		chansStopOnSeqHalt_0 = new gRadio(x()+25, y()+125, 280, 20, "play all dynamic channels until finished");
	radioGrp_2->end();

	treatRecsAsLoops  = new gCheck(x(), y()+155, 280, 20, "Treat one shot channels with actions as loops");
	fullChanVolOnLoad = new gCheck(x(), y()+185, 280, 20, "Bring channels to full volume on sample load");
	end();
	labelsize(11);

	G_Conf.recsStopOnChanHalt == 1 ? recsStopOnChanHalt_1->value(1) : recsStopOnChanHalt_0->value(1);
	G_Conf.chansStopOnSeqHalt == 1 ? chansStopOnSeqHalt_1->value(1) : chansStopOnSeqHalt_0->value(1);
	G_Conf.treatRecsAsLoops   == 1 ? treatRecsAsLoops->value(1)  : treatRecsAsLoops->value(0);
	G_Conf.fullChanVolOnLoad  == 1 ? fullChanVolOnLoad->value(1) : fullChanVolOnLoad->value(0);

	recsStopOnChanHalt_1->callback(cb_radio_mutex, (void*)this);
	recsStopOnChanHalt_0->callback(cb_radio_mutex, (void*)this);
	chansStopOnSeqHalt_1->callback(cb_radio_mutex, (void*)this);
	chansStopOnSeqHalt_0->callback(cb_radio_mutex, (void*)this);
}





void gTabBehaviors::cb_radio_mutex(Fl_Widget *w, void *p) { ((gTabBehaviors*)p)->__cb_radio_mutex(w); }





void gTabBehaviors::__cb_radio_mutex(Fl_Widget *w) {
	((Fl_Button *)w)->type(FL_RADIO_BUTTON);
}





void gTabBehaviors::save() {
	G_Conf.recsStopOnChanHalt = recsStopOnChanHalt_1->value() == 1 ? 1 : 0;
	G_Conf.chansStopOnSeqHalt = chansStopOnSeqHalt_1->value() == 1 ? 1 : 0;
	G_Conf.treatRecsAsLoops   = treatRecsAsLoops->value() == 1 ? 1 : 0;
	G_Conf.fullChanVolOnLoad  = fullChanVolOnLoad->value() == 1 ? 1 : 0;
}







gdConfig::gdConfig(int w, int h) : gWindow(w, h, "Configuration")
{
	set_modal();

	if (G_Conf.configX)
		resize(G_Conf.configX, G_Conf.configY, this->w(), this->h());

	Fl_Tabs *tabs = new Fl_Tabs(8, 8, w-16, h-44);
		tabAudio     = new gTabAudio(tabs->x()+10, tabs->y()+20, tabs->w()-20, tabs->h()-40);
		tabMidi      = new gTabMidi(tabs->x()+10, tabs->y()+20, tabs->w()-20, tabs->h()-40);
		tabBehaviors = new gTabBehaviors(tabs->x()+10, tabs->y()+20, tabs->w()-20, tabs->h()-40);
	tabs->end();

	save 	 = new gClick (w-88, h-28, 80, 20, "Save");
	cancel = new gClick (w-176, h-28, 80, 20, "Cancel");

	end();

	tabs->box(G_BOX);
	tabs->labelcolor(COLOR_TEXT_0);

	save->callback(cb_save_config, (void*)this);
	cancel->callback(cb_cancel, (void*)this);

	gu_setFavicon(this);
	setId(WID_CONFIG);
	show();
}





gdConfig::~gdConfig() {
	G_Conf.configX = x();
	G_Conf.configY = y();
}





void gdConfig::cb_save_config(Fl_Widget *w, void *p) { ((gdConfig*)p)->__cb_save_config(); }
void gdConfig::cb_cancel     (Fl_Widget *w, void *p) { ((gdConfig*)p)->__cb_cancel(); }





void gdConfig::__cb_save_config() {
	tabAudio->save();
	tabBehaviors->save();
	tabMidi->save();
	do_callback();
}





void gdConfig::__cb_cancel() {
	do_callback();
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "gd_devInfo.h"
#include "ge_mixed.h"
#include "kernelAudio.h"
#include "gui_utils.h"


gdDevInfo::gdDevInfo(unsigned dev)
: Fl_Window(340, 300, "Device information") {
	set_modal();

	text  = new gBox(8, 8, 320, 200, "", (Fl_Align) (FL_ALIGN_LEFT | FL_ALIGN_TOP));
	close = new gClick(252, h()-28, 80, 20, "Close");
	end();

	std::string bufTxt;
	char bufNum[128];
	int  lines = 0;

	bufTxt  = "Device name: ";
	bufTxt += +kernelAudio::getDeviceName(dev);
	bufTxt += "\n";
	lines++;

	bufTxt += "Total output(s): ";
	sprintf(bufNum, "%d\n", kernelAudio::getMaxOutChans(dev));
	bufTxt += bufNum;
	lines++;

	bufTxt += "Total intput(s): ";
	sprintf(bufNum, "%d\n", kernelAudio::getMaxInChans(dev));
	bufTxt += bufNum;
	lines++;

	bufTxt += "Duplex channel(s): ";
	sprintf(bufNum, "%d\n", kernelAudio::getDuplexChans(dev));
	bufTxt += bufNum;
	lines++;

	bufTxt += "Default output: ";
	sprintf(bufNum, "%s\n", kernelAudio::isDefaultOut(dev) ? "yes" : "no");
	bufTxt += bufNum;
	lines++;

	bufTxt += "Default input: ";
	sprintf(bufNum, "%s\n", kernelAudio::isDefaultIn(dev) ? "yes" : "no");
	bufTxt += bufNum;
	lines++;

	int totalFreq = kernelAudio::getTotalFreqs(dev);
	bufTxt += "Supported frequencies: ";
	sprintf(bufNum, "%d", totalFreq);
	bufTxt += bufNum;
	lines++;

	for (int i=0; i<totalFreq; i++) {
		sprintf(bufNum, "%d  ", kernelAudio::getFreq(dev, i));
		if (i%6 == 0) {    
			bufTxt += "\n    ";
			lines++;
		}
		bufTxt += bufNum;
	}

	text->copy_label(bufTxt.c_str());

	

	resize(x(), y(), w(), lines*fl_height() + 8 + 8 + 8 + 20);
	close->position(close->x(), lines*fl_height() + 8 + 8);

	close->callback(__cb_window_closer, (void*)this);
	gu_setFavicon(this);
	show();
}


gdDevInfo::~gdDevInfo() {}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "gd_editor.h"
#include "gd_mainWindow.h"
#include "ge_waveform.h"
#include "waveFx.h"
#include "conf.h"
#include "gui_utils.h"
#include "glue.h"
#include "gd_warnings.h"
#include "mixerHandler.h"
#include "ge_mixed.h"
#include "gg_waveTools.h"
#include "channel.h"
#include "sampleChannel.h"
#include "mixer.h"
#include "wave.h"


extern Mixer         G_Mixer;
extern gdMainWindow *mainWin;
extern Conf	         G_Conf;


gdEditor::gdEditor(SampleChannel *ch)
	: gWindow(640, 480),
		ch(ch)
{
	set_non_modal();

	if (G_Conf.sampleEditorX)
		resize(G_Conf.sampleEditorX, G_Conf.sampleEditorY, G_Conf.sampleEditorW, G_Conf.sampleEditorH);

	Fl_Group *bar = new Fl_Group(8, 8, w()-16, 20);
	bar->begin();
		reload  = new gClick(bar->x(), bar->y(), 50, 20, "Reload");
		zoomOut = new gClick(bar->x()+bar->w()-20, bar->y(), 20, 20, "-");
		zoomIn  = new gClick(zoomOut->x()-24, bar->y(), 20, 20, "+");
	bar->end();
	bar->resizable(new gBox(reload->x()+reload->w()+4, bar->y(), 80, bar->h()));

	waveTools = new gWaveTools(8, 36, w()-16, h()-120, ch);
	waveTools->end();

	Fl_Group *tools = new Fl_Group(8, waveTools->y()+waveTools->h()+8, w()-16, 130);
	tools->begin();
		volume        = new gDial (tools->x()+42,	                   tools->y(), 20, 20, "Volume");
		volumeNum     = new gInput(volume->x()+volume->w()+4,        tools->y(), 46, 20, "dB");

		boost         = new gDial (volumeNum->x()+volumeNum->w()+80, tools->y(), 20, 20, "Boost");
		boostNum      = new gInput(boost->x()+boost->w()+4,          tools->y(), 46, 20, "dB");

		normalize     = new gClick(boostNum->x()+boostNum->w()+40,   tools->y(), 70, 20, "Normalize");
		pan 				  = new gDial (normalize->x()+normalize->w()+40, tools->y(), 20, 20, "Pan");
		panNum    	  = new gInput(pan->x()+pan->w()+4,              tools->y(), 45, 20, "%");

		pitch				  = new gDial  (tools->x()+42,	                     volume->y()+volume->h()+4, 20, 20, "Pitch");
		pitchNum		  = new gInput (pitch->x()+pitch->w()+4,	           volume->y()+volume->h()+4, 46, 20);
		pitchToBar    = new gClick (pitchNum->x()+pitchNum->w()+4,       volume->y()+volume->h()+4, 46, 20, "To bar");
		pitchToSong   = new gClick (pitchToBar->x()+pitchToBar->w()+4,   volume->y()+volume->h()+4, 46, 20, "To song");
		pitchHalf     = new gClick (pitchToSong->x()+pitchToSong->w()+4, volume->y()+volume->h()+4, 21, 20, "÷");
		pitchDouble   = new gClick (pitchHalf->x()+pitchHalf->w()+4,     volume->y()+volume->h()+4, 21, 20, "×");
		pitchReset    = new gClick (pitchDouble->x()+pitchDouble->w()+4, volume->y()+volume->h()+4, 46, 20, "Reset");

		chanStart     = new gInput(tools->x()+52,                    pitch->y()+pitch->h()+4, 60, 20, "Start");
		chanEnd       = new gInput(chanStart->x()+chanStart->w()+40, pitch->y()+pitch->h()+4, 60, 20, "End");
		resetStartEnd = new gClick(chanEnd->x()+chanEnd->w()+4,      pitch->y()+pitch->h()+4, 46, 20, "Reset");

	tools->end();
	tools->resizable(new gBox(chanStart->x()+chanStart->w()+4, tools->y(), 80, tools->h()));

	char buf[16];
	
	sprintf(buf, "%d", ch->begin / 2); 
	chanStart->value(buf);
	chanStart->type(FL_INT_INPUT);
	chanStart->callback(cb_setChanPos, this);

	

	chanStart->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	chanEnd  ->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	
	sprintf(buf, "%d", ch->end / 2);  
	chanEnd->value(buf);
	chanEnd->type(FL_INT_INPUT);
	chanEnd->callback(cb_setChanPos, this);

	resetStartEnd->callback(cb_resetStartEnd, this);

	volume->callback(cb_setVolume, (void*)this);
	volume->value(ch->guiChannel->vol->value());

	float dB = 20*log10(ch->volume);   
	if (dB > -INFINITY)	sprintf(buf, "%.2f", dB);
	else            		sprintf(buf, "-inf");
	volumeNum->value(buf);
	volumeNum->align(FL_ALIGN_RIGHT);
	volumeNum->callback(cb_setVolumeNum, (void*)this);

	boost->range(1.0f, 10.0f);
	boost->callback(cb_setBoost, (void*)this);
	if (ch->boost > 10.f)
		boost->value(10.0f);
	else
		boost->value(ch->boost);
	boost->when(FL_WHEN_CHANGED | FL_WHEN_RELEASE);

	float boost = 20*log10(ch->boost); 
	sprintf(buf, "%.2f", boost);
	boostNum->value(buf);
	boostNum->align(FL_ALIGN_RIGHT);
	boostNum->callback(cb_setBoostNum, (void*)this);

	normalize->callback(cb_normalize, (void*)this);

	pan->range(0.0f, 2.0f);
	pan->callback(cb_panning, (void*)this);

	pitch->range(0.01f, 4.0f);
	pitch->value(ch->pitch);
	pitch->callback(cb_setPitch, (void*)this);
	pitch->when(FL_WHEN_RELEASE);

	sprintf(buf, "%.4f", ch->pitch); 
	pitchNum->value(buf);
	pitchNum->align(FL_ALIGN_RIGHT);
	pitchNum->callback(cb_setPitchNum, (void*)this);
	pitchNum->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	pitchToBar->callback(cb_setPitchToBar, (void*)this);
	pitchToSong->callback(cb_setPitchToSong, (void*)this);
	pitchHalf->callback(cb_setPitchHalf, (void*)this);
	pitchDouble->callback(cb_setPitchDouble, (void*)this);
	pitchReset->callback(cb_resetPitch, (void*)this);

	reload->callback(cb_reload, (void*)this);

	zoomOut->callback(cb_zoomOut, (void*)this);
	zoomIn->callback(cb_zoomIn, (void*)this);

	

	if (ch->wave->isLogical)
		reload->deactivate();

	if (ch->panRight < 1.0f) {
		char buf[8];
		sprintf(buf, "%d L", abs((ch->panRight * 100.0f) - 100));
		pan->value(ch->panRight);
		panNum->value(buf);
	}
	else if (ch->panRight == 1.0f && ch->panLeft == 1.0f) {
	  pan->value(1.0f);
	  panNum->value("C");
	}
	else {
		char buf[8];
		sprintf(buf, "%d R", abs((ch->panLeft * 100.0f) - 100));
		pan->value(2.0f - ch->panLeft);
		panNum->value(buf);
	}

	panNum->align(FL_ALIGN_RIGHT);
	panNum->readonly(1);
	panNum->cursor_color(FL_WHITE);

	gu_setFavicon(this);
	size_range(640, 480);
	resizable(waveTools);

	label(ch->wave->name.c_str());

	show();
}





gdEditor::~gdEditor() {
	G_Conf.sampleEditorX = x();
	G_Conf.sampleEditorY = y();
	G_Conf.sampleEditorW = w();
	G_Conf.sampleEditorH = h();
}





void gdEditor::cb_setChanPos      (Fl_Widget *w, void *p) { ((gdEditor*)p)->__cb_setChanPos(); }
void gdEditor::cb_resetStartEnd   (Fl_Widget *w, void *p) { ((gdEditor*)p)->__cb_resetStartEnd(); }
void gdEditor::cb_setVolume       (Fl_Widget *w, void *p) { ((gdEditor*)p)->__cb_setVolume(); }
void gdEditor::cb_setVolumeNum    (Fl_Widget *w, void *p) { ((gdEditor*)p)->__cb_setVolumeNum(); }
void gdEditor::cb_setBoost        (Fl_Widget *w, void *p) { ((gdEditor*)p)->__cb_setBoost(); }
void gdEditor::cb_setBoostNum     (Fl_Widget *w, void *p) { ((gdEditor*)p)->__cb_setBoostNum(); }
void gdEditor::cb_normalize       (Fl_Widget *w, void *p) { ((gdEditor*)p)->__cb_normalize(); }
void gdEditor::cb_panning         (Fl_Widget *w, void *p) { ((gdEditor*)p)->__cb_panning(); }
void gdEditor::cb_reload          (Fl_Widget *w, void *p) { ((gdEditor*)p)->__cb_reload(); }
void gdEditor::cb_setPitch        (Fl_Widget *w, void *p) { ((gdEditor*)p)->__cb_setPitch(); }
void gdEditor::cb_setPitchToBar   (Fl_Widget *w, void *p) { ((gdEditor*)p)->__cb_setPitchToBar(); }
void gdEditor::cb_setPitchToSong  (Fl_Widget *w, void *p) { ((gdEditor*)p)->__cb_setPitchToSong(); }
void gdEditor::cb_setPitchHalf    (Fl_Widget *w, void *p) { ((gdEditor*)p)->__cb_setPitchHalf(); }
void gdEditor::cb_setPitchDouble  (Fl_Widget *w, void *p) { ((gdEditor*)p)->__cb_setPitchDouble(); }
void gdEditor::cb_resetPitch      (Fl_Widget *w, void *p) { ((gdEditor*)p)->__cb_resetPitch(); }
void gdEditor::cb_setPitchNum     (Fl_Widget *w, void *p) { ((gdEditor*)p)->__cb_setPitchNum(); }
void gdEditor::cb_zoomIn          (Fl_Widget *w, void *p) { ((gdEditor*)p)->__cb_zoomIn(); }
void gdEditor::cb_zoomOut         (Fl_Widget *w, void *p) { ((gdEditor*)p)->__cb_zoomOut(); }





void gdEditor::__cb_setPitchToBar() {
	glue_setPitch(this, ch, ch->end/(float)G_Mixer.framesPerBar, true);
}





void gdEditor::__cb_setPitchToSong() {
	glue_setPitch(this, ch, ch->end/(float)G_Mixer.totalFrames, true);
}





void gdEditor::__cb_resetPitch() {
	glue_setPitch(this, ch, 1.0f, true);
}





void gdEditor::__cb_setChanPos() {
	glue_setBeginEndChannel(
		this,
		ch,
		atoi(chanStart->value())*2,  
		atoi(chanEnd->value())*2,
		true
	);
}





void gdEditor::__cb_resetStartEnd() {
	glue_setBeginEndChannel(this, ch, 0, ch->wave->size, true);
}





void gdEditor::__cb_setVolume() {
	glue_setVolEditor(this, ch, volume->value(), false);
}





void gdEditor::__cb_setVolumeNum() {
	glue_setVolEditor(this, ch, atof(volumeNum->value()), true);
}





void gdEditor::__cb_setBoost() {
	if (Fl::event() == FL_DRAG)
		glue_setBoost(this, ch, boost->value(), false);
	else if (Fl::event() == FL_RELEASE) {
		glue_setBoost(this, ch, boost->value(), false);
	waveTools->updateWaveform();
	}
}





void gdEditor::__cb_setBoostNum() {
	glue_setBoost(this, ch, atof(boostNum->value()), true);
	waveTools->updateWaveform();
}





void gdEditor::__cb_normalize() {
	float val = wfx_normalizeSoft(ch->wave);
	glue_setBoost(this, ch, val, false); 
	if (val < 0.0f)
		boost->value(0.0f);
	else
	if (val > 20.0f) 
		boost->value(10.0f);
	else
		boost->value(val);
	waveTools->updateWaveform();
}





void gdEditor::__cb_panning() {
	glue_setPanning(this, ch, pan->value());
}





void gdEditor::__cb_reload() {

	if (!gdConfirmWin("Warning", "Reload sample: are you sure?"))
		return;

	

	ch->load(ch->wave->pathfile.c_str());

	glue_setBoost(this, ch, DEFAULT_BOOST, true);
	glue_setPitch(this, ch, gDEFAULT_PITCH, true);
	glue_setPanning(this, ch, 1.0f);
	pan->value(1.0f);  
	pan->redraw();     

	waveTools->waveform->stretchToWindow();
	waveTools->updateWaveform();

	glue_setBeginEndChannel(this, ch, 0, ch->wave->size, true);

	redraw();
}





void gdEditor::__cb_setPitch() {
	glue_setPitch(this, ch, pitch->value(), false);
}





void gdEditor::__cb_setPitchNum() {
	glue_setPitch(this, ch, atof(pitchNum->value()), true);
}





void gdEditor::__cb_setPitchHalf() {
	glue_setPitch(this, ch, pitch->value()/2, true);
}





void gdEditor::__cb_setPitchDouble() {
	glue_setPitch(this, ch, pitch->value()*2, true);
}





void gdEditor::__cb_zoomIn() {
	waveTools->waveform->setZoom(-1);
	waveTools->redraw();
}





void gdEditor::__cb_zoomOut() {
	waveTools->waveform->setZoom(0);
	waveTools->redraw();
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "gd_keyGrabber.h"
#include "ge_mixed.h"
#include "conf.h"
#include "gui_utils.h"
#include "gd_config.h"
#include "channel.h"
#include "sampleChannel.h"
#include "midiChannel.h"
#include "gg_keyboard.h"


extern Conf	G_Conf;


gdKeyGrabber::gdKeyGrabber(SampleChannel *ch)
: Fl_Window(300, 100, "Key configuration"), ch(ch) {
	set_modal();
	text = new gBox(10, 10, 280, 80, "Press a key (esc to quit):");
	gu_setFavicon(this);
	show();
}





int gdKeyGrabber::handle(int e) {
	int ret = Fl_Group::handle(e);
	switch(e) {
		case FL_KEYUP: {
			int x = Fl::event_key();
			if (strlen(Fl::event_text()) != 0
			    && x != FL_BackSpace
			    && x != FL_Enter
			    && x != FL_Delete
			    && x != FL_Tab
			    && x != FL_End
			    && x != ' ')
			{
				printf("set key '%c' (%d) for channel %d\n", x, x, ch->index);

				char tmp[2]; sprintf(tmp, "%c", x);
				ch->guiChannel->button->copy_label(tmp);
				ch->key = x;
				Fl::delete_widget(this);
				break;
			}
			else
				printf("invalid key\n");
		}
	}
	return(ret);
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "gd_mainWindow.h"
#include "graphics.h"
#include "gd_warnings.h"
#include "glue.h"
#include "mixer.h"
#include "recorder.h"
#include "gd_browser.h"
#include "mixerHandler.h"
#include "pluginHost.h"
#include "channel.h"
#include "sampleChannel.h"
#include "gd_midiGrabber.h"


extern Mixer	   		 G_Mixer;
extern Patch     		 G_Patch;
extern Conf	 	   		 G_Conf;
extern gdMainWindow *mainWin;
extern bool	 		 		 G_quit;
extern bool 		 		 G_audio_status;

#if defined(WITH_VST)
extern PluginHost  	 G_PluginHost;
#endif


gdMainWindow::gdMainWindow(int X, int Y, int W, int H, const char *title, int argc, char **argv)
: gWindow(X, Y, W, H, title) {

	Fl::visible_focus(0);
	Fl::background(25, 25, 25);
	Fl::set_boxtype(G_BOX, gDrawBox, 1, 1, 2, 2);    

	begin();

	menu_file 	= new gClick(8,   -1, 70, 21, "file");
	menu_edit	  = new gClick(82,  -1, 70, 21, "edit");
	menu_config	= new gClick(156, -1, 70, 21, "config");
	menu_about	= new gClick(230, -1, 70, 21, "about");

	quantize    = new gChoice(632, 49, 40, 15, "", false);
	bpm         = new gClick(676,  49, 40, 15);
	beats       = new gClick(724,  49, 40, 15, "4/1");
	beats_mul   = new gClick(768,  49, 15, 15, "×");
	beats_div   = new gClick(787,  49, 15, 15, "÷");

#if defined(WITH_VST)
	masterFxIn  = new gButton(408, 8, 20, 20, "", fxOff_xpm, fxOn_xpm);
	inVol		    = new gDial  (432, 8, 20, 20);
	inMeter     = new gSoundMeter(456, 13, 140, 10);
	inToOut     = new gClick (600, 13, 10, 10, "");
	outMeter    = new gSoundMeter(614, 13, 140, 10);
	outVol		  = new gDial  (758, 8, 20, 20);
	masterFxOut = new gButton(782, 8, 20, 20, "", fxOff_xpm, fxOn_xpm);
#else
	outMeter    = new gSoundMeter(638, 13, 140, 10);
	inMeter     = new gSoundMeter(494, 13, 140, 10);
	outVol		  = new gDial(782, 8, 20, 20);
	inVol		    = new gDial(470, 8, 20, 20);
#endif

	beatMeter   = new gBeatMeter(100, 83, 609, 20);

	beat_rew		= new gClick(8,  39, 25, 25, "", rewindOff_xpm, rewindOn_xpm);
	beat_stop		= new gClick(37, 39, 25, 25, "", play_xpm, pause_xpm);
	beat_rec		= new gClick(66, 39, 25, 25, "", recOff_xpm, recOn_xpm);
	input_rec		= new gClick(95, 39, 25, 25, "", inputRecOff_xpm, inputRecOn_xpm);

	metronome   = new gClick(124, 49, 15, 15, "", metronomeOff_xpm, metronomeOn_xpm);

	keyboard    = new Keyboard(8, 122, w()-16, 380);

	

	end();

	char buf_bpm[6]; snprintf(buf_bpm, 6, "%f", G_Mixer.bpm);
	bpm->copy_label(buf_bpm);
	bpm->callback(cb_change_bpm);
	beats->callback(cb_change_batt);

	menu_about->callback(cb_open_about_win);
	menu_file->callback(cb_open_file_menu);
	menu_edit->callback(cb_open_edit_menu);
	menu_config->callback(cb_open_config_win);

	outVol->callback(cb_outVol);
	outVol->value(G_Mixer.outVol);
	inVol->callback(cb_inVol);
	inVol->value(G_Mixer.inVol);
#ifdef WITH_VST
	masterFxOut->callback(cb_openMasterFxOut);
	masterFxIn->callback(cb_openMasterFxIn);
	inToOut->callback(cb_inToOut);
	inToOut->type(FL_TOGGLE_BUTTON);
#endif

	beat_rew->callback(cb_rewind_tracker);
	beat_stop->callback(cb_startstop);
	beat_stop->type(FL_TOGGLE_BUTTON);

	beat_rec->callback(cb_rec);
	beat_rec->type(FL_TOGGLE_BUTTON);
	input_rec->callback(cb_inputRec);
	input_rec->type(FL_TOGGLE_BUTTON);

	beats_mul->callback(cb_beatsMultiply);
	beats_div->callback(cb_beatsDivide);

	metronome->type(FL_TOGGLE_BUTTON);
	metronome->callback(cb_metronome);

	callback(cb_endprogram);

	quantize->add("off", 0, cb_quantize, (void*)0);
	quantize->add("1b",  0, cb_quantize, (void*)1);
	quantize->add("2b",  0, cb_quantize, (void*)2);
	quantize->add("3b",  0, cb_quantize, (void*)3);
	quantize->add("4b",  0, cb_quantize, (void*)4);
	quantize->add("6b",  0, cb_quantize, (void*)6);
	quantize->add("8b",  0, cb_quantize, (void*)8);
	quantize->value(0); 

	gu_setFavicon(this);
	show(argc, argv);
}


gdMainWindow::~gdMainWindow() {}






void gdMainWindow::cb_endprogram     (Fl_Widget *v, void *p)    { mainWin->__cb_endprogram(); }
void gdMainWindow::cb_change_bpm     (Fl_Widget *v, void *p)    { mainWin->__cb_change_bpm(); }
void gdMainWindow::cb_change_batt    (Fl_Widget *v, void *p) 		{ mainWin->__cb_change_batt(); }
void gdMainWindow::cb_open_about_win (Fl_Widget *v, void *p) 		{ mainWin->__cb_open_about_win(); }
void gdMainWindow::cb_rewind_tracker (Fl_Widget *v, void *p) 		{ mainWin->__cb_rewind_tracker(); }
void gdMainWindow::cb_open_config_win(Fl_Widget *v, void *p) 		{ mainWin->__cb_open_config_win(); }
void gdMainWindow::cb_startstop      (Fl_Widget *v, void *p)  	{ mainWin->__cb_startstop(); }
void gdMainWindow::cb_rec            (Fl_Widget *v, void *p) 		{ mainWin->__cb_rec(); }
void gdMainWindow::cb_inputRec       (Fl_Widget *v, void *p) 		{ mainWin->__cb_inputRec(); }
void gdMainWindow::cb_quantize       (Fl_Widget *v, void *p)  	{ mainWin->__cb_quantize((intptr_t)p); }
void gdMainWindow::cb_outVol         (Fl_Widget *v, void *p)  	{ mainWin->__cb_outVol(); }
void gdMainWindow::cb_inVol          (Fl_Widget *v, void *p)  	{ mainWin->__cb_inVol(); }
void gdMainWindow::cb_open_file_menu (Fl_Widget *v, void *p)  	{ mainWin->__cb_open_file_menu(); }
void gdMainWindow::cb_open_edit_menu (Fl_Widget *v, void *p)  	{ mainWin->__cb_open_edit_menu(); }
void gdMainWindow::cb_metronome      (Fl_Widget *v, void *p)    { mainWin->__cb_metronome(); }
void gdMainWindow::cb_beatsMultiply  (Fl_Widget *v, void *p)    { mainWin->__cb_beatsMultiply(); }
void gdMainWindow::cb_beatsDivide    (Fl_Widget *v, void *p)    { mainWin->__cb_beatsDivide(); }
#ifdef WITH_VST
void gdMainWindow::cb_openMasterFxOut(Fl_Widget *v, void *p)    { mainWin->__cb_openMasterFxOut(); }
void gdMainWindow::cb_openMasterFxIn (Fl_Widget *v, void *p)    { mainWin->__cb_openMasterFxIn(); }
void gdMainWindow::cb_inToOut        (Fl_Widget *v, void *p)    { mainWin->__cb_inToOut(); }
#endif





void gdMainWindow::__cb_endprogram() {

	if (!gdConfirmWin("Warning", "Quit Giada: are you sure?"))
		return;

	G_quit = true;

	

	puts("GUI closing...");
	gu_closeAllSubwindows();

	

	if (!G_Conf.write())
		puts("Error while saving configuration file!");
	else
		puts("Configuration saved");

	puts("Mixer cleanup...");

	

	if (G_audio_status) {
		kernelAudio::closeDevice();
		G_Mixer.close();
	}

	puts("Recorder cleanup...");
	recorder::clearAll();

#ifdef WITH_VST
	puts("Plugin Host cleanup...");
	G_PluginHost.freeAllStacks();
#endif

	puts("Giada "VERSIONE" closed.");
	hide();

	delete this;
}





void gdMainWindow::__cb_change_bpm() {
	gu_openSubWindow(mainWin, new gdBpmInput(), WID_BPM);
}





void gdMainWindow::__cb_change_batt() {
	gu_openSubWindow(mainWin, new gdBeatsInput(), WID_BEATS);
}





void gdMainWindow::__cb_open_about_win() {
	gu_openSubWindow(mainWin, new gdAbout(), WID_ABOUT);
}





void gdMainWindow::__cb_open_loadpatch_win() {
	gWindow *childWin = new gdBrowser("Load Patch", G_Conf.patchPath, 0, BROWSER_LOAD_PATCH);
	gu_openSubWindow(mainWin, childWin, WID_FILE_BROWSER);
}





void gdMainWindow::__cb_open_savepatch_win() {
	gWindow *childWin = new gdBrowser("Save Patch", G_Conf.patchPath, 0, BROWSER_SAVE_PATCH);
	gu_openSubWindow(mainWin, childWin, WID_FILE_BROWSER);
}





void gdMainWindow::__cb_open_saveproject_win() {
	gWindow *childWin = new gdBrowser("Save Project", G_Conf.patchPath, 0, BROWSER_SAVE_PROJECT);
	gu_openSubWindow(mainWin, childWin, WID_FILE_BROWSER);
}





void gdMainWindow::__cb_open_config_win() {
	gu_openSubWindow(mainWin, new gdConfig(380, 370), WID_CONFIG);
}





void gdMainWindow::__cb_rewind_tracker() {
	glue_rewindSeq();
}





void gdMainWindow::__cb_startstop() {
	glue_startStopSeq();
}





void gdMainWindow::__cb_rec() {
	glue_startStopActionRec();
}





void gdMainWindow::__cb_inputRec() {
	glue_startStopInputRec();
}





void gdMainWindow::__cb_quantize(int v) {
	glue_quantize(v);
}





void gdMainWindow::__cb_outVol() {
	glue_setOutVol(outVol->value());
}





void gdMainWindow::__cb_inVol() {
	glue_setInVol(inVol->value());
}





void gdMainWindow::__cb_open_file_menu() {

	

	Fl_Menu_Item menu[] = {
		{"Open patch or project..."},
		{"Save patch..."},
		{"Save project..."},
		{"Quit Giada"},
		{0}
	};

	Fl_Menu_Button *b = new Fl_Menu_Button(0, 0, 100, 50);
	b->box(G_BOX);
	b->textsize(11);
	b->textcolor(COLOR_TEXT_0);
	b->color(COLOR_BG_0);

	const Fl_Menu_Item *m = menu->popup(Fl::event_x(),	Fl::event_y(), 0, 0, b);
	if (!m) return;


	if (strcmp(m->label(), "Open patch or project...") == 0) {
		__cb_open_loadpatch_win();
		return;
	}
	if (strcmp(m->label(), "Save patch...") == 0) {
		if (G_Mixer.hasLogicalSamples() || G_Mixer.hasEditedSamples())
			if (!gdConfirmWin("Warning", "You should save a project in order to store\nyour takes and/or processed samples."))
				return;
		__cb_open_savepatch_win();
		return;
	}
	if (strcmp(m->label(), "Save project...") == 0) {
		__cb_open_saveproject_win();
		return;
	}
	if (strcmp(m->label(), "Quit Giada") == 0) {
		__cb_endprogram();
		return;
	}
}





void gdMainWindow::__cb_open_edit_menu() {
	Fl_Menu_Item menu[] = {
		{"Clear all samples"},
		{"Clear all actions"},
		{"Reset to init state"},
		{"Setup global MIDI input..."},
		{0}
	};

	

	menu[1].deactivate();

	for (unsigned i=0; i<G_Mixer.channels.size; i++)
		if (G_Mixer.channels.at(i)->hasActions) {
			menu[1].activate();
			break;
		}
	for (unsigned i=0; i<G_Mixer.channels.size; i++)
		if (G_Mixer.channels.at(i)->type == CHANNEL_SAMPLE)
			if (((SampleChannel*)G_Mixer.channels.at(i))->wave != NULL) {
				menu[0].activate();
				break;
			}

	Fl_Menu_Button *b = new Fl_Menu_Button(0, 0, 100, 50);
	b->box(G_BOX);
	b->textsize(11);
	b->textcolor(COLOR_TEXT_0);
	b->color(COLOR_BG_0);

	const Fl_Menu_Item *m = menu->popup(Fl::event_x(),	Fl::event_y(), 0, 0, b);
	if (!m) return;

	if (strcmp(m->label(), "Clear all samples") == 0) {
		if (!gdConfirmWin("Warning", "Clear all samples: are you sure?"))
			return;
		delSubWindow(WID_SAMPLE_EDITOR);
		glue_clearAllSamples();
		return;
	}
	if (strcmp(m->label(), "Clear all actions") == 0) {
		if (!gdConfirmWin("Warning", "Clear all actions: are you sure?"))
			return;
		delSubWindow(WID_ACTION_EDITOR);
		glue_clearAllRecs();
		return;
	}
	if (strcmp(m->label(), "Reset to init state") == 0) {
		if (!gdConfirmWin("Warning", "Reset to init state: are you sure?"))
			return;
		gu_closeAllSubwindows();
		glue_resetToInitState();
		return;
	}
	if (strcmp(m->label(), "Setup global MIDI input...") == 0) {
		gu_openSubWindow(mainWin, new gdMidiGrabberMaster(), 0);
		return;
	}
}





void gdMainWindow::__cb_metronome() {
	glue_startStopMetronome();
}





void gdMainWindow::__cb_beatsMultiply() {
	glue_beatsMultiply();
}





void gdMainWindow::__cb_beatsDivide() {
	glue_beatsDivide();
}





#ifdef WITH_VST
void gdMainWindow::__cb_openMasterFxOut() {
	gu_openSubWindow(mainWin, new gdPluginList(PluginHost::MASTER_OUT), WID_FX_LIST);
}

void gdMainWindow::__cb_openMasterFxIn() {
	gu_openSubWindow(mainWin, new gdPluginList(PluginHost::MASTER_IN), WID_FX_LIST);
}

void gdMainWindow::__cb_inToOut() {
	G_Mixer.inToOut = inToOut->value();
}
#endif

/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "gd_midiGrabber.h"
#include "ge_mixed.h"
#include "gui_utils.h"
#include "kernelMidi.h"
#include "conf.h"
#include "sampleChannel.h"


extern Conf G_Conf;


gdMidiGrabber::gdMidiGrabber(int w, int h, const char *title)
	: gWindow(w, h, title)
{
}





gdMidiGrabber::~gdMidiGrabber() {
	kernelMidi::stopMidiLearn();
}





void gdMidiGrabber::stopMidiLearn(gLearner *learner) {
	kernelMidi::stopMidiLearn();
	learner->updateValue();
}





void gdMidiGrabber::__cb_learn(uint32_t *param, uint32_t msg, gLearner *l) {
	*param = msg;
	stopMidiLearn(l);
	printf("[gdMidiGrabber] MIDI learn done - message=0x%X\n", msg);
}





void gdMidiGrabber::cb_learn(uint32_t msg, void *d) {
	cbData *data = (cbData*) d;
	gdMidiGrabber *grabber = (gdMidiGrabber*) data->grabber;
	gLearner      *learner = data->learner;
	uint32_t      *param   = learner->param;
	grabber->__cb_learn(param, msg, learner);
	free(data);
}





void gdMidiGrabber::cb_close(Fl_Widget *w, void *p)  { ((gdMidiGrabber*)p)->__cb_close(); }





void gdMidiGrabber::__cb_close() {
	do_callback();
}







gdMidiGrabberChannel::gdMidiGrabberChannel(Channel *ch)
	:	gdMidiGrabber(300, 206, "MIDI Input Setup"),
		ch(ch)
{
	char title[64];
	sprintf(title, "MIDI Input Setup (channel %d)", ch->index);
	label(title);

	set_modal();

	enable = new gCheck(8, 8, 120, 20, "enable MIDI input");
	new gLearner(8,  30, w()-16, "key press",   cb_learn, &ch->midiInKeyPress);
	new gLearner(8,  54, w()-16, "key release", cb_learn, &ch->midiInKeyRel);
	new gLearner(8,  78, w()-16, "key kill",    cb_learn, &ch->midiInKill);
	new gLearner(8, 102, w()-16, "mute",        cb_learn, &ch->midiInMute);
	new gLearner(8, 126, w()-16, "solo",        cb_learn, &ch->midiInSolo);
	new gLearner(8, 150, w()-16, "volume",      cb_learn, &ch->midiInVolume);
	int yy = 178;

	if (ch->type == CHANNEL_SAMPLE) {
		size(300, 254);
		new gLearner(8, 174, w()-16, "pitch", cb_learn, &((SampleChannel*)ch)->midiInPitch);
		new gLearner(8, 198, w()-16, "read actions", cb_learn, &((SampleChannel*)ch)->midiInReadActions);
		yy = 226;
	}

	ok = new gButton(w()-88, yy, 80, 20, "Ok");
	ok->callback(cb_close, (void*)this);

	enable->value(ch->midiIn);
	enable->callback(cb_enable, (void*)this);

	gu_setFavicon(this);
	show();
}





void gdMidiGrabberChannel::cb_enable(Fl_Widget *w, void *p)  { ((gdMidiGrabberChannel*)p)->__cb_enable(); }





void gdMidiGrabberChannel::__cb_enable() {
	ch->midiIn = enable->value();
}







gdMidiGrabberMaster::gdMidiGrabberMaster()
	: gdMidiGrabber(300, 256, "MIDI Input Setup (global)")
{
	set_modal();

	new gLearner(8,   8, w()-16, "rewind",           &cb_learn, &G_Conf.midiInRewind);
	new gLearner(8,  32, w()-16, "play/stop",        &cb_learn, &G_Conf.midiInStartStop);
	new gLearner(8,  56, w()-16, "action recording", &cb_learn, &G_Conf.midiInActionRec);
	new gLearner(8,  80, w()-16, "input recording",  &cb_learn, &G_Conf.midiInInputRec);
	new gLearner(8, 104, w()-16, "metronome",        &cb_learn, &G_Conf.midiInMetronome);
	new gLearner(8, 128, w()-16, "input volume",     &cb_learn, &G_Conf.midiInVolumeIn);
	new gLearner(8, 152, w()-16, "output volume",    &cb_learn, &G_Conf.midiInVolumeOut);
	new gLearner(8, 176, w()-16, "sequencer ×2",     &cb_learn, &G_Conf.midiInBeatDouble);
	new gLearner(8, 200, w()-16, "sequencer ÷2",     &cb_learn, &G_Conf.midiInBeatHalf);
	ok = new gButton(w()-88, 228, 80, 20, "Ok");

	ok->callback(cb_close, (void*)this);

	gu_setFavicon(this);
	show();
}







gLearner::gLearner(int X, int Y, int W, const char *l, kernelMidi::cb_midiLearn *cb, uint32_t *param)
	: Fl_Group(X, Y, W, 20),
		callback(cb),
		param   (param)
{
	begin();
	text   = new gBox(x(), y(), 156, 20, l);
	value  = new gClick(text->x()+text->w()+4, y(), 80, 20, "(not set)");
	button = new gButton(value->x()+value->w()+4, y(), 40, 20, "learn");
	end();

	text->box(G_BOX);
	text->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	value->box(G_BOX);
	value->callback(cb_value, (void*)this);
	value->when(FL_WHEN_RELEASE);
	updateValue();

	button->type(FL_TOGGLE_BUTTON);
	button->callback(cb_button, (void*)this);
}





void gLearner::updateValue() {
	char buf[16];
	if (*param != 0x0)
		snprintf(buf, 9, "0x%X", *param);
	else
		snprintf(buf, 16, "(not set)");
	value->copy_label(buf);
	button->value(0);
}





void gLearner::cb_button(Fl_Widget *v, void *p) { ((gLearner*)p)->__cb_button(); }
void gLearner::cb_value(Fl_Widget *v, void *p) { ((gLearner*)p)->__cb_value(); }





void gLearner::__cb_value() {
	if (Fl::event_button() == FL_RIGHT_MOUSE) {
		*param = 0x0;
		updateValue();
	}
	
}





void gLearner::__cb_button() {
	if (button->value() == 1) {
		cbData *data  = (cbData*) malloc(sizeof(cbData));
		data->grabber = (gdMidiGrabber*) parent();  
		data->learner = this;
		kernelMidi::startMidiLearn(callback, (void*)data);
	}
	else
		kernelMidi::stopMidiLearn();
}

/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "gd_midiOutputSetup.h"
#include "conf.h"
#include "ge_mixed.h"
#include "gg_keyboard.h"
#include "channel.h"
#include "midiChannel.h"
#include "gui_utils.h"


extern Conf	G_Conf;


gdMidiOutputSetup::gdMidiOutputSetup(MidiChannel *ch)
	: gWindow(300, 64, "Midi Output Setup"), ch(ch)
{
	begin();
	enableOut      = new gCheck(x()+8, y()+8, 150, 20, "Enable MIDI output");
	chanListOut    = new gChoice(w()-108, y()+8, 100, 20);

	save   = new gButton(w()-88, chanListOut->y()+chanListOut->h()+8, 80, 20, "Save");
	cancel = new gButton(w()-88-save->w()-8, save->y(), 80, 20, "Cancel");
	end();

	fillChanMenu(chanListOut);

	if (ch->midiOut)
		enableOut->value(1);
	else
		chanListOut->deactivate();

	chanListOut->value(ch->midiOutChan);

	enableOut->callback(cb_enableChanList, (void*)this);
	save->callback(cb_save, (void*)this);
	cancel->callback(cb_cancel, (void*)this);

	set_modal();
	gu_setFavicon(this);
	show();
}





void gdMidiOutputSetup::cb_save          (Fl_Widget *w, void *p) { ((gdMidiOutputSetup*)p)->__cb_save(); }
void gdMidiOutputSetup::cb_cancel        (Fl_Widget *w, void *p) { ((gdMidiOutputSetup*)p)->__cb_cancel(); }
void gdMidiOutputSetup::cb_enableChanList(Fl_Widget *w, void *p) { ((gdMidiOutputSetup*)p)->__cb_enableChanList(); }





void gdMidiOutputSetup::__cb_enableChanList() {
	enableOut->value() ? chanListOut->activate() : chanListOut->deactivate();
}





void gdMidiOutputSetup::__cb_save() {
	ch->midiOut     = enableOut->value();
	ch->midiOutChan = chanListOut->value();
	ch->guiChannel->update();
	do_callback();
}





void gdMidiOutputSetup::__cb_cancel() { do_callback(); }





void gdMidiOutputSetup::fillChanMenu(gChoice *m) {
	m->add("Channel 1");
	m->add("Channel 2");
	m->add("Channel 3");
	m->add("Channel 4");
	m->add("Channel 5");
	m->add("Channel 6");
	m->add("Channel 7");
	m->add("Channel 8");
	m->add("Channel 9");
	m->add("Channel 10");
	m->add("Channel 11");
	m->add("Channel 12");
	m->add("Channel 13");
	m->add("Channel 14");
	m->add("Channel 15");
	m->add("Channel 16");
	m->value(0);
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#ifdef WITH_VST

#include "gd_pluginList.h"
#include "utils.h"
#include "gd_pluginWindow.h"
#include "gd_pluginWindowGUI.h"
#include "conf.h"
#include "gui_utils.h"
#include "gd_browser.h"
#include "graphics.h"
#include "pluginHost.h"
#include "ge_mixed.h"
#include "mixer.h"
#include "channel.h"


extern Conf       G_Conf;
extern PluginHost G_PluginHost;


gdPluginList::gdPluginList(int stackType, Channel *ch)
 : gWindow(468, 204), ch(ch), stackType(stackType)
{

	if (G_Conf.pluginListX)
		resize(G_Conf.pluginListX, G_Conf.pluginListY, w(), h());

	list = new Fl_Scroll(8, 8, 476, 188);
	list->type(Fl_Scroll::VERTICAL);
	list->scrollbar.color(COLOR_BG_0);
	list->scrollbar.selection_color(COLOR_BG_1);
	list->scrollbar.labelcolor(COLOR_BD_1);
	list->scrollbar.slider(G_BOX);

	list->begin();
		refreshList();
	list->end();

	end();
	set_non_modal();

	if (stackType == PluginHost::MASTER_OUT)
		label("Master Out Plugins");
	else
	if (stackType == PluginHost::MASTER_IN)
		label("Master In Plugins");
	else {
		char tmp[32];
		sprintf(tmp, "Channel %d Plugins", ch->index+1);
		copy_label(tmp);
	}

	gu_setFavicon(this);
	show();
}





gdPluginList::~gdPluginList() {
	G_Conf.pluginListX = x();
	G_Conf.pluginListY = y();
}





void gdPluginList::cb_addPlugin(Fl_Widget *v, void *p)   { ((gdPluginList*)p)->__cb_addPlugin(); }





void gdPluginList::cb_refreshList(Fl_Widget *v, void *p) {

	

	gWindow *child = (gWindow*) v;
	if (child->getParent() != NULL)
		(child->getParent())->delSubWindow(child);

	

	((gdPluginList*)p)->refreshList();
	((gdPluginList*)p)->redraw();
}





void gdPluginList::__cb_addPlugin() {

	

	gdBrowser *b = new gdBrowser("Browse Plugin", G_Conf.pluginPath, ch, BROWSER_LOAD_PLUGIN, stackType);
	addSubWindow(b);
	b->callback(cb_refreshList, (void*)this);	

}





void gdPluginList::refreshList() {

	

	list->clear();
	list->scroll_to(0, 0);

	

	int numPlugins = G_PluginHost.countPlugins(stackType, ch);
	int i = 0;

	while (i<numPlugins) {
		Plugin   *pPlugin  = G_PluginHost.getPluginByIndex(i, stackType, ch);
		gdPlugin *gdp      = new gdPlugin(this, pPlugin, list->x(), list->y()-list->yposition()+(i*24), 800);
		list->add(gdp);
		i++;
	}

	int addPlugY = numPlugins == 0 ? 90 : list->y()-list->yposition()+(i*24);
	addPlugin = new gClick(8, addPlugY, 452, 20, "-- add new plugin --");
	addPlugin->callback(cb_addPlugin, (void*)this);
	list->add(addPlugin);

	

	if (i>7)
		size(492, h());
	else
		size(468, h());

	redraw();
}







gdPlugin::gdPlugin(gdPluginList *gdp, Plugin *p, int X, int Y, int W)
	: Fl_Group(X, Y, W, 20), pParent(gdp), pPlugin (p)
{
	begin();
	button    = new gButton(8, y(), 220, 20);
	program   = new gChoice(button->x()+button->w()+4, y(), 132, 20);
	bypass    = new gButton(program->x()+program->w()+4, y(), 20, 20);
	shiftUp   = new gButton(bypass->x()+bypass->w()+4, y(), 20, 20, "", fxShiftUpOff_xpm, fxShiftUpOn_xpm);
	shiftDown = new gButton(shiftUp->x()+shiftUp->w()+4, y(), 20, 20, "", fxShiftDownOff_xpm, fxShiftDownOn_xpm);
	remove    = new gButton(shiftDown->x()+shiftDown->w()+4, y(), 20, 20, "", fxRemoveOff_xpm, fxRemoveOn_xpm);
	end();

	if (pPlugin->status != 1) {  
		char name[256];
		sprintf(name, "* %s *", gBasename(pPlugin->pathfile).c_str());
		button->copy_label(name);
	}
	else {
		char name[256];
		pPlugin->getProduct(name);
		if (strcmp(name, " ")==0)
			pPlugin->getName(name);

		button->copy_label(name);
		button->callback(cb_openPluginWindow, (void*)this);

		program->callback(cb_setProgram, (void*)this);

		
		

		for (int i=0; i<64; i++) {
			char out[kVstMaxProgNameLen];
			pPlugin->getProgramName(i, out);
			for (int j=0; j<kVstMaxProgNameLen; j++)  
				if (out[j] == '/' || out[j] == '\\' || out[j] == '&' || out[j] == '_')
					out[j] = '-';
			if (strlen(out) > 0)
				program->add(out);
		}
		if (program->size() == 0) {
			program->add("-- no programs --\0");
			program->deactivate();
		}
		if (pPlugin->getProgram() == -1)
			program->value(0);
		else
			program->value(pPlugin->getProgram());

		bypass->callback(cb_setBypass, (void*)this);
		bypass->type(FL_TOGGLE_BUTTON);
		bypass->value(pPlugin->bypass ? 0 : 1);
	}

	shiftUp->callback(cb_shiftUp, (void*)this);
	shiftDown->callback(cb_shiftDown, (void*)this);
	remove->callback(cb_removePlugin, (void*)this);
}





void gdPlugin::cb_removePlugin    (Fl_Widget *v, void *p)    { ((gdPlugin*)p)->__cb_removePlugin(); }
void gdPlugin::cb_openPluginWindow(Fl_Widget *v, void *p)    { ((gdPlugin*)p)->__cb_openPluginWindow(); }
void gdPlugin::cb_setBypass       (Fl_Widget *v, void *p)    { ((gdPlugin*)p)->__cb_setBypass(); }
void gdPlugin::cb_shiftUp         (Fl_Widget *v, void *p)    { ((gdPlugin*)p)->__cb_shiftUp(); }
void gdPlugin::cb_shiftDown       (Fl_Widget *v, void *p)    { ((gdPlugin*)p)->__cb_shiftDown(); }
void gdPlugin::cb_setProgram      (Fl_Widget *v, void *p)    { ((gdPlugin*)p)->__cb_setProgram(); }





void gdPlugin::__cb_shiftUp() {

	

	if (G_PluginHost.countPlugins(pParent->stackType, pParent->ch) == 1)
		return;

	int pluginIndex = G_PluginHost.getPluginIndex(pPlugin->getId(), pParent->stackType, pParent->ch);

	if (pluginIndex == 0)  
		return;

	G_PluginHost.swapPlugin(pluginIndex, pluginIndex-1, pParent->stackType, pParent->ch);
	pParent->refreshList();
}





void gdPlugin::__cb_shiftDown() {

	

	if (G_PluginHost.countPlugins(pParent->stackType, pParent->ch) == 1)
		return;

	unsigned pluginIndex = G_PluginHost.getPluginIndex(pPlugin->getId(), pParent->stackType, pParent->ch);
	unsigned stackSize   = (G_PluginHost.getStack(pParent->stackType, pParent->ch))->size;

	if (pluginIndex == stackSize-1)  
		return;

	G_PluginHost.swapPlugin(pluginIndex, pluginIndex+1, pParent->stackType, pParent->ch);
	pParent->refreshList();
}





void gdPlugin::__cb_removePlugin() {

	

#ifdef __APPLE__
	gdPluginWindowGUImac* w = (gdPluginWindowGUImac*) pParent->getChild(pPlugin->getId()+1);
	if (w)
		w->show();
#endif

	

	pParent->delSubWindow(pPlugin->getId()+1);
	G_PluginHost.freePlugin(pPlugin->getId(), pParent->stackType, pParent->ch);
	pParent->refreshList();
}





void gdPlugin::__cb_openPluginWindow() {

	

	

	if (!pParent->hasWindow(pPlugin->getId()+1)) {
		gWindow *w;
		if (pPlugin->hasGui())
#ifdef __APPLE__
			w = new gdPluginWindowGUImac(pPlugin);
#else
			w = new gdPluginWindowGUI(pPlugin);
#endif
		else
			w = new gdPluginWindow(pPlugin);
		w->setId(pPlugin->getId()+1);
		pParent->addSubWindow(w);
	}
}





void gdPlugin::__cb_setBypass() {
	pPlugin->bypass = !pPlugin->bypass;
}





void gdPlugin::__cb_setProgram() {
	pPlugin->setProgram(program->value());
}



#endif 
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#ifdef WITH_VST

#include <FL/Fl_Scroll.H>
#include "gd_pluginWindow.h"
#include "pluginHost.h"
#include "ge_mixed.h"
#include "gui_utils.h"


extern PluginHost G_PluginHost;


Parameter::Parameter(int id, Plugin *p, int X, int Y, int W)
	: Fl_Group(X,Y,W-24,20), id(id), pPlugin(p)
{
	begin();

		label = new gBox(x(), y(), 60, 20);
		char name[kVstMaxParamStrLen];
		pPlugin->getParamName(id, name);
		label->copy_label(name);
		label->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

		slider = new gSlider(label->x()+label->w()+8, y(), W-200, 20);
		slider->value(pPlugin->getParam(id));
		slider->callback(cb_setValue, (void *)this);

		value = new gBox(slider->x()+slider->w()+8, y(), 100, 20);
		char disp[kVstMaxParamStrLen];
		char labl[kVstMaxParamStrLen];
		char str [256];
		pPlugin->getParamDisplay(id, disp);
		pPlugin->getParamLabel(id, labl);
		sprintf(str, "%s %s", disp, labl);
		value->copy_label(str);
		value->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
		value->box(G_BOX);

		resizable(slider);

	end();
}





void Parameter::cb_setValue(Fl_Widget *v, void *p)  { ((Parameter*)p)->__cb_setValue(); }





void Parameter::__cb_setValue() {

	pPlugin->setParam(id, slider->value());

	char disp[256];
	char labl[256];
	char str [256];

	pPlugin->getParamDisplay(id, disp);
	pPlugin->getParamLabel(id, labl);
	sprintf(str, "%s %s", disp, labl);

	value->copy_label(str);
	value->redraw();
}





gdPluginWindow::gdPluginWindow(Plugin *pPlugin)
 : gWindow(400, 156), pPlugin(pPlugin) 
{
	set_non_modal();

	gLiquidScroll *list = new gLiquidScroll(8, 8, w()-16, h()-16);
	list->type(Fl_Scroll::VERTICAL_ALWAYS);
	list->begin();

	int numParams = pPlugin->getNumParams();
	for (int i=0; i<numParams; i++)
		new Parameter(i, pPlugin, list->x(), list->y()+(i*24), list->w());
	list->end();

	end();

	char name[256];
	pPlugin->getProduct(name);
	if (strcmp(name, " ")==0)
		pPlugin->getName(name);
	label(name);

	size_range(400, (24*1)+12);
	resizable(list);

	gu_setFavicon(this);
	show();

}


gdPluginWindow::~gdPluginWindow() {}


#endif 
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#ifdef WITH_VST


#include "gd_pluginWindowGUI.h"
#include "pluginHost.h"
#include "ge_mixed.h"
#include "gui_utils.h"


extern PluginHost G_PluginHost;


gdPluginWindowGUI::gdPluginWindowGUI(Plugin *pPlugin)
 : gWindow(450, 300), pPlugin(pPlugin)
{

  

  ERect *rect;
	pPlugin->getRect(&rect);

	gu_setFavicon(this);
	set_non_modal();
	resize(x(), y(), pPlugin->getGuiWidth(), pPlugin->getGuiHeight());
	show();

	

	Fl::check();
	pPlugin->openGui((void*)fl_xid(this));

	char name[256];
	pPlugin->getProduct(name);
	copy_label(name);

	

	pPlugin->window = this;

	pPlugin->idle();
}





gdPluginWindowGUI::~gdPluginWindowGUI() {
	pPlugin->closeGui();
}







#if defined(__APPLE__)


pascal OSStatus gdPluginWindowGUImac::windowHandler(EventHandlerCallRef ehc, EventRef e, void *data) {
	return ((gdPluginWindowGUImac*)data)->__wh(ehc, e);
}





pascal OSStatus gdPluginWindowGUImac::__wh(EventHandlerCallRef inHandlerCallRef, EventRef inEvent) {
	OSStatus result   = eventNotHandledErr;     
	UInt32 eventClass = GetEventClass(inEvent);
	UInt32 eventKind  = GetEventKind(inEvent);

	switch (eventClass)	{
		case kEventClassWindow:	{
			switch (eventKind) {
				case kEventWindowClose:	{
					printf("[pluginWindowMac] <<< CALLBACK >>> kEventWindowClose for gWindow=%p, window=%p\n", (void*)this, (void*)carbonWindow);
					show();
					break;
				}
				case kEventWindowClosed: {
					printf("[pluginWindowMac] <<< CALLBACK >>> kEventWindowClosed for gWindow=%p, window=%p\n", (void*)this, (void*)carbonWindow);
					open = false;
					result = noErr;
					break;
				}
			}
			break;
		}
	}
	return result;
}





gdPluginWindowGUImac::gdPluginWindowGUImac(Plugin *pPlugin)
 : gWindow(450, 300), pPlugin(pPlugin), carbonWindow(NULL)
{

  

  ERect *rect;
	pPlugin->getRect(&rect);

	

	Rect wRect;

	wRect.top    = rect->top;
	wRect.left   = rect->left;
	wRect.bottom = rect->bottom;
	wRect.right  = rect->right;

  int winclass = kDocumentWindowClass;
  int winattr  = kWindowStandardHandlerAttribute |
                 kWindowCloseBoxAttribute        |
                 kWindowCompositingAttribute     |
                 kWindowAsyncDragAttribute;

  

  OSStatus status = CreateNewWindow(winclass, winattr, &wRect, &carbonWindow);
	if (status != noErr)	{
		printf("[pluginWindowMac] Unable to create window! Status=%d\n", (int) status);
		return;
	}
	else
		printf("[pluginWindowMac] created window=%p\n", (void*)carbonWindow);

	

	static EventTypeSpec eventTypes[] = {
		{ kEventClassWindow, kEventWindowClose },
		{ kEventClassWindow, kEventWindowClosed }
	};
	InstallWindowEventHandler(carbonWindow, windowHandler, GetEventTypeCount(eventTypes), eventTypes, this, NULL);

	

	pPlugin->openGui((void*)carbonWindow);
	RepositionWindow(carbonWindow, NULL, kWindowCenterOnMainScreen);
	ShowWindow(carbonWindow);
	open = true;
}






gdPluginWindowGUImac::~gdPluginWindowGUImac() {
	printf("[pluginWindowMac] [[[ destructor ]]] gWindow=%p deleted, window=%p deleted\n", (void*)this, (void*)carbonWindow);
	pPlugin->closeGui();
	if (open)
		DisposeWindow(carbonWindow);
}

#endif

#endif 
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "gd_warnings.h"


void gdAlert(const char *c) {
	Fl_Window *modal = new Fl_Window(
			(Fl::w() / 2) - 150,
			(Fl::h() / 2) - 47,
			300, 90, "Alert");
	modal->set_modal();
	modal->begin();
		gBox *box = new gBox(10, 10, 280, 40, c);
		gClick *b = new gClick(210, 60, 80, 20, "Close");
	modal->end();
	box->labelsize(11);
	b->callback(__cb_window_closer, (void *)modal);
	b->shortcut(FL_Enter);
	gu_setFavicon(modal);
	modal->show();
}


int gdConfirmWin(const char *title, const char *msg) {
	Fl_Window *win = new Fl_Window(
			(Fl::w() / 2) - 150,
			(Fl::h() / 2) - 47,
			300, 90, title);
	win->set_modal();
	win->begin();
		new gBox(10, 10, 280, 40, msg);
		gClick *ok = new gClick(212, 62, 80, 20, "Ok");
		gClick *ko = new gClick(124, 62, 80, 20, "Cancel");
	win->end();
	ok->shortcut(FL_Enter);
	gu_setFavicon(win);
	win->show();

	

	int r = 0;
	while (true) {
		Fl_Widget *o = Fl::readqueue();
		if (!o) Fl::wait();
		else if (o == ok) {r = 1; break;}
		else if (o == ko) {r = 0; break;}
	}
	
	win->hide();
	return r;
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include <FL/fl_draw.H>
#include "glue.h"
#include "ge_actionChannel.h"
#include "gd_mainWindow.h"
#include "gd_actionEditor.h"
#include "conf.h"
#include "channel.h"
#include "sampleChannel.h"


extern gdMainWindow *mainWin;
extern Mixer         G_Mixer;
extern Conf	         G_Conf;





gActionChannel::gActionChannel(int x, int y, gdActionEditor *pParent, SampleChannel *ch)
 : gActionWidget(x, y, 200, 40, pParent), ch(ch), selected(NULL)
{
	size(pParent->totalWidth, h());

	

	for (unsigned i=0; i<recorder::frames.size; i++) {
		for (unsigned j=0; j<recorder::global.at(i).size; j++) {

			recorder::action *ra = recorder::global.at(i).at(j);

			if (ra->chan == pParent->chan->index) {

				

				if (recorder::frames.at(i) > G_Mixer.totalFrames)
					continue;

				

				if (ra->type == ACTION_KILLCHAN && ch->mode == SINGLE_PRESS)
					continue;

				

				if (ra->type & (ACTION_KEYPRESS | ACTION_KILLCHAN))	{
					int ax = x+(ra->frame/pParent->zoom);
					gAction *a = new gAction(
							ax,           
							y+4,          
							h()-8,        
							ra->frame,	  
							i,            
							pParent,      
							ch,           
							false,        
							ra->type);    
					add(a);
				}
			}
		}
	}
	end(); 
}





gAction *gActionChannel::getSelectedAction() {
	for (int i=0; i<children(); i++) {
		int action_x  = ((gAction*)child(i))->x();
		int action_w  = ((gAction*)child(i))->w();
		if (Fl::event_x() >= action_x && Fl::event_x() <= action_x + action_w)
			return (gAction*)child(i);
	}
	return NULL;
}





void gActionChannel::updateActions() {

	

	gAction *a;
	for (int i=0; i<children(); i++) {

		a = (gAction*)child(i);
		int newX = x() + (a->frame_a / pParent->zoom);

		if (ch->mode == SINGLE_PRESS) {
			int newW = ((a->frame_b - a->frame_a) / pParent->zoom);
			if (newW < gAction::MIN_WIDTH)
				newW = gAction::MIN_WIDTH;
			a->resize(newX, a->y(), newW, a->h());
		}
		else
			a->resize(newX, a->y(), gAction::MIN_WIDTH, a->h());
	}
}





void gActionChannel::draw() {

	

	baseDraw();

	

	fl_color(COLOR_BG_1);
	fl_font(FL_HELVETICA, 12);
	if (active())
		fl_draw("start/stop", x()+4, y(), w(), h(), (Fl_Align) (FL_ALIGN_LEFT | FL_ALIGN_CENTER));  
	else
		fl_draw("start/stop (disabled)", x()+4, y(), w(), h(), (Fl_Align) (FL_ALIGN_LEFT | FL_ALIGN_CENTER));  

	draw_children();
}





int gActionChannel::handle(int e) {

	int ret = Fl_Group::handle(e);

	

	if (!active())
		return 1;

	switch (e) {

		case FL_DRAG: {
			if (selected != NULL) {   

				

				if (selected->onLeftEdge || selected->onRightEdge) {

					

					if (selected->onRightEdge) {

						int aw = Fl::event_x()-selected->x();
						int ah = selected->h();

						if (Fl::event_x() < selected->x()+gAction::MIN_WIDTH)
							aw = gAction::MIN_WIDTH;
						else
						if (Fl::event_x() > pParent->coverX)
							aw = pParent->coverX-selected->x();

						selected->size(aw, ah);
					}
					else {

						int ax = Fl::event_x();
						int ay = selected->y();
						int aw = selected->x()-Fl::event_x()+selected->w();
						int ah = selected->h();

						if (Fl::event_x() < x()) {
							ax = x();
							aw = selected->w()+selected->x()-x();
						}
						else
						if (Fl::event_x() > selected->x()+selected->w()-gAction::MIN_WIDTH) {
							ax = selected->x()+selected->w()-gAction::MIN_WIDTH;
							aw = gAction::MIN_WIDTH;
						}
						selected->resize(ax, ay, aw, ah);
					}
				}

				

				else {
					int real_x = Fl::event_x() - actionPickPoint;
					if (real_x < x())                                  
						selected->position(x(), selected->y());
					else
					if (real_x+selected->w() > pParent->coverX+x())         
						selected->position(pParent->coverX+x()-selected->w(), selected->y());
					else {
						if (pParent->gridTool->isOn()) {
							int snpx = pParent->gridTool->getSnapPoint(real_x-x()) + x() -1;
							selected->position(snpx, selected->y());
						}
						else
							selected->position(real_x, selected->y());
					}
				}
				redraw();
			}
			ret = 1;
			break;
		}

		case FL_PUSH:	{

			if (Fl::event_button1()) {

				

				selected = getSelectedAction();

				if (selected == NULL) {

					

					if (Fl::event_x() >= pParent->coverX) {
						ret = 1;
						break;
					}

					

					int ax = Fl::event_x();
					int fx = (ax - x()) * pParent->zoom;
					if (pParent->gridTool->isOn()) {
						ax = pParent->gridTool->getSnapPoint(ax-x()) + x() -1;
						fx = pParent->gridTool->getSnapFrame(ax-x());

						

						if (actionCollides(fx)) {
							ret = 1;
							break;
						}
					}

					gAction *a = new gAction(
							ax,                                   
							y()+4,                                
							h()-8,                                
							fx,																		
							recorder::frames.size-1,              
							pParent,                              
							ch,                                   
							true,                                 
							pParent->getActionType());            
					add(a);
					mainWin->keyboard->setChannelWithActions((gSampleChannel*)ch->guiChannel); 
					redraw();
					ret = 1;
				}
				else {
					actionOriginalX = selected->x();
					actionOriginalW = selected->w();
					actionPickPoint = Fl::event_x() - selected->x();
					ret = 1;   
				}
			}
			else
			if (Fl::event_button3()) {
				gAction *a = getSelectedAction();
				if (a != NULL) {
					a->delAction();
					remove(a);
					delete a;
					mainWin->keyboard->setChannelWithActions((gSampleChannel*)pParent->chan->guiChannel);
					redraw();
					ret = 1;
				}
			}
			break;
		}
		case FL_RELEASE: {

			if (selected == NULL) {
				ret = 1;
				break;
			}

			

			bool noChanges = false;
			if (actionOriginalX == selected->x())
				noChanges = true;
			if (ch->mode == SINGLE_PRESS &&
					actionOriginalX+actionOriginalW != selected->x()+selected->w())
				noChanges = false;

			if (noChanges) {
				ret = 1;
				selected = NULL;
				break;
			}

			

			bool overlap = false;
			for (int i=0; i<children() && !overlap; i++) {

				

				if ((gAction*)child(i) == selected)
					continue;

				int action_x  = ((gAction*)child(i))->x();
				int action_w  = ((gAction*)child(i))->w();
				if (ch->mode == SINGLE_PRESS) {

					

					 int start = action_x >= selected->x() ? action_x : selected->x();
					 int end   = action_x+action_w < selected->x()+selected->w() ? action_x+action_w : selected->x()+selected->w();
					 if (start < end) {
						selected->resize(actionOriginalX, selected->y(), actionOriginalW, selected->h());
						redraw();
						overlap = true;   
					}
				}
				else {
					if (selected->x() == action_x) {
						selected->position(actionOriginalX, selected->y());
						redraw();
						overlap = true;   
					}
				}
			}

			

			if (!overlap) {
				if (pParent->gridTool->isOn()) {
					int f = pParent->gridTool->getSnapFrame(selected->absx());
					selected->moveAction(f);
				}
				else
					selected->moveAction();
			}
			selected = NULL;
			ret = 1;
			break;
		}
	}

	return ret;
}





bool gActionChannel::actionCollides(int frame) {

	

	bool collision = false;

	for (int i=0; i<children() && !collision; i++)
		if ( ((gAction*)child(i))->frame_a == frame)
			collision = true;

	if (ch->mode == SINGLE_PRESS) {
		for (int i=0; i<children() && !collision; i++) {
			gAction *c = ((gAction*)child(i));
			if (frame <= c->frame_b && frame >= c->frame_a)
				collision = true;
		}
	}

	return collision;
}







const int gAction::MIN_WIDTH = 8;







gAction::gAction(int X, int Y, int H, int frame_a, unsigned index, gdActionEditor *parent, SampleChannel *ch, bool record, char type)
: Fl_Box     (X, Y, MIN_WIDTH, H),
  selected   (false),
  index      (index),
  parent     (parent),
  ch         (ch),
  type       (type),
  frame_a    (frame_a),
  onRightEdge(false),
  onLeftEdge (false)
{
	

	if (record)
		addAction();

	

	if (ch->mode == SINGLE_PRESS && type == ACTION_KEYPRESS) {
		recorder::action *a2 = NULL;
		recorder::getNextAction(ch->index, ACTION_KEYREL, frame_a, &a2);
		if (a2) {
			frame_b = a2->frame;
			w((frame_b - frame_a)/parent->zoom);
		}
		else
			printf("[gActionChannel] frame_b not found! [%d:???]\n", frame_a);

	

		if (w() < MIN_WIDTH)
			size(MIN_WIDTH, h());
	}
}





void gAction::draw() {

	int color;
	if (selected)  
		color = COLOR_BD_1;
	else
		color = COLOR_BG_2;

	if (ch->mode == SINGLE_PRESS) {
		fl_rectf(x(), y(), w(), h(), (Fl_Color) color);
	}
	else {
		if (type == ACTION_KILLCHAN)
			fl_rect(x(), y(), MIN_WIDTH, h(), (Fl_Color) color);
		else {
			fl_rectf(x(), y(), MIN_WIDTH, h(), (Fl_Color) color);
			if (type == ACTION_KEYPRESS)
				fl_rectf(x()+3, y()+h()-11, 2, 8, COLOR_BD_0);
			else
			if  (type == ACTION_KEYREL)
				fl_rectf(x()+3, y()+3, 2, 8, COLOR_BD_0);
		}
	}

}





int gAction::handle(int e) {

	

	int ret = 0;

	switch (e) {

		case FL_ENTER: {
			selected = true;
			ret = 1;
			redraw();
			break;
		}
		case FL_LEAVE: {
			fl_cursor(FL_CURSOR_DEFAULT, FL_WHITE, FL_BLACK);
			selected = false;
			ret = 1;
			redraw();
			break;
		}
		case FL_MOVE: {

			

			if (ch->mode == SINGLE_PRESS) {
				onLeftEdge  = false;
				onRightEdge = false;
				if (Fl::event_x() >= x() && Fl::event_x() < x()+4) {
					onLeftEdge = true;
					fl_cursor(FL_CURSOR_WE, FL_WHITE, FL_BLACK);
				}
				else
				if (Fl::event_x() >= x()+w()-4 && Fl::event_x() <= x()+w()) {
					onRightEdge = true;
					fl_cursor(FL_CURSOR_WE, FL_WHITE, FL_BLACK);
				}
				else
					fl_cursor(FL_CURSOR_DEFAULT, FL_WHITE, FL_BLACK);
			}
		}
	}

	return ret;
}





void gAction::addAction() {

	

	if (frame_a % 2 != 0)
		frame_a++;

	

	if (ch->mode == SINGLE_PRESS) {
		recorder::rec(parent->chan->index, ACTION_KEYPRESS, frame_a);
		recorder::rec(parent->chan->index, ACTION_KEYREL, frame_a+4096);
		
	}
	else {
		recorder::rec(parent->chan->index, parent->getActionType(), frame_a);
		
	}

	recorder::sortActions();

	index++; 
}





void gAction::delAction() {

	

	if (ch->mode == SINGLE_PRESS) {
		recorder::deleteAction(parent->chan->index, frame_a, ACTION_KEYPRESS, false);
		recorder::deleteAction(parent->chan->index, frame_b, ACTION_KEYREL, false);
	}
	else
		recorder::deleteAction(parent->chan->index, frame_a, type, false);

	

	fl_cursor(FL_CURSOR_DEFAULT, FL_WHITE, FL_BLACK);
}





void gAction::moveAction(int frame_a) {

	

	delAction();

	if (frame_a != -1)
		this->frame_a = frame_a;
	else
		this->frame_a = xToFrame_a();


	

	if (this->frame_a % 2 != 0)
		this->frame_a++;

	recorder::rec(parent->chan->index, type, this->frame_a);

	if (ch->mode == SINGLE_PRESS) {
		frame_b = xToFrame_b();
		recorder::rec(parent->chan->index, ACTION_KEYREL, frame_b);
	}

	recorder::sortActions();
}





int gAction::absx() {
	return x() - parent->ac->x();
}





int gAction::xToFrame_a() {
	return (absx()) * parent->zoom;
}





int gAction::xToFrame_b() {
	return (absx() + w()) * parent->zoom;
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include <FL/fl_draw.H>
#include "ge_actionWidget.h"
#include "gd_actionEditor.h"
#include "mixer.h"
#include "ge_mixed.h"


extern Mixer G_Mixer;


gActionWidget::gActionWidget(int x, int y, int w, int h, gdActionEditor *pParent)
	:	Fl_Group(x, y, w, h), pParent(pParent) {}





gActionWidget::~gActionWidget() {}





void gActionWidget::baseDraw(bool clear) {

	

	if (clear)
		fl_rectf(x(), y(), w(), h(), COLOR_BG_MAIN);

	

	fl_color(COLOR_BD_0);
	fl_rect(x(), y(), w(), h());

	

	if (pParent->gridTool->getValue() > 1) {

		fl_color(fl_rgb_color(54, 54, 54));
		fl_line_style(FL_DASH, 0, NULL);

		for (int i=0; i<(int) pParent->gridTool->points.size; i++) {
			int px = pParent->gridTool->points.at(i)+x()-1;
			fl_line(px, y()+1, px, y()+h()-2);
		}
		fl_line_style(0);
	}

	

	fl_color(COLOR_BD_0);
	for (int i=0; i<(int) pParent->gridTool->beats.size; i++) {
		int px = pParent->gridTool->beats.at(i)+x()-1;
		fl_line(px, y()+1, px, y()+h()-2);
	}

	fl_color(COLOR_BG_2);
	for (int i=0; i<(int) pParent->gridTool->bars.size; i++) {
		int px = pParent->gridTool->bars.at(i)+x()-1;
		fl_line(px, y()+1, px, y()+h()-2);
	}

	

	int coverWidth = pParent->totalWidth-pParent->coverX;
	if (coverWidth != 0)
		fl_rectf(pParent->coverX+x(), y()+1, coverWidth, h()-2, COLOR_BG_1);
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include <limits.h>
#include "ge_browser.h"
#include "const.h"
#include "utils.h"


gBrowser::gBrowser(int x, int y, int w, int h, const char *L)
 : Fl_Hold_Browser(x, y, w, h, L)
{
	box(G_BOX);
	textsize(11);
	textcolor(COLOR_TEXT_0);
	selection_color(COLOR_BG_1);
	color(COLOR_BG_0);

	this->scrollbar.color(COLOR_BG_0);
	this->scrollbar.selection_color(COLOR_BG_1);
	this->scrollbar.labelcolor(COLOR_BD_1);
	this->scrollbar.slider(G_BOX);

	this->hscrollbar.color(COLOR_BG_0);
	this->hscrollbar.selection_color(COLOR_BG_1);
	this->hscrollbar.labelcolor(COLOR_BD_1);
	this->hscrollbar.slider(G_BOX);
}





gBrowser::~gBrowser() {}





void gBrowser::init(const char *init_path) {

	printf("[gBrowser] init path = '%s'\n", init_path);

	if (init_path == NULL || !gIsDir(init_path)) {
#if defined(__linux__) || defined(__APPLE__)
		path_obj->value("/home");
#elif defined(_WIN32)

		

		char winRoot[1024];
		SHGetFolderPath(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL, 0, winRoot); 
		path_obj->value(winRoot);
#endif
		puts("[gBrowser] init_path null or invalid, using default");
	}
	else
		path_obj->value(init_path);

	refresh();
	sort();
}





void gBrowser::refresh() {
  DIR *dp;
  struct dirent *ep;
  dp = opendir(path_obj->value());
  if (dp != NULL) {
		while ((ep = readdir(dp))) {

			

			if (strcmp(ep->d_name, ".") != 0 && strcmp(ep->d_name, "..") != 0) {
				if (ep->d_name[0] != '.') {

					

					std::string file = path_obj->value();
					file.insert(file.size(), gGetSlash());
					file += ep->d_name;

					if (gIsDir(file.c_str())) {
						char name[PATH_MAX];
						sprintf(name, "@b[%s]", ep->d_name);
						add(name);
					}
					else
					if (gIsProject(file.c_str())) {
						char name[PATH_MAX];
						sprintf(name, "@i@b%s", ep->d_name);
						add(name);
					}
					else
						add(ep->d_name);
				}
			}
		}
		closedir(dp);
  }
  else
    printf("[gBrowser] Couldn't open the directory '%s'\n", path_obj->value());
}





void gBrowser::sort() {
	for (int t=1; t<=size(); t++)
		for (int r=t+1; r<=size(); r++)
			if (strcmp(text(t), text(r)) > 0)
				swap(t,r);
}





void gBrowser::up_dir() {

	

	int i = strlen(path_obj->value())-1;

	

#if defined(_WIN32)
	if (i <= 3 || !strcmp(path_obj->value(), "All drives")) {
		path_obj->value("All drives");
		showDrives();
		return;
	}
	else {
		while (i >= 0) {
			if (path_obj->value()[i] == '\\')
				break;
			i--;
		}

		

		std::string tmp = path_obj->value();
		tmp.erase(i, tmp.size()-i);

		

		if (tmp.size() == 2)
			tmp += "\\";

		path_obj->value(tmp.c_str());
		refresh();
	}
#elif defined(__linux__) || defined (__APPLE__)
	while (i >= 0) {
		if (path_obj->value()[i] == '/')
			break;
		i--;
	}

	

	if (i==0)
		path_obj->value("/");
	else {

		

		std::string tmp = path_obj->value();
		tmp.erase(i, tmp.size()-i);
		path_obj->value(tmp.c_str());
	}
	refresh();
#endif
}





void gBrowser::down_dir(const char *path) {
	path_obj->value(path);
	refresh();
}





const char *gBrowser::get_selected_item() {

	

	if (text(value()) == NULL)
		return NULL;

	selected_item = text(value());

	

	if (selected_item[0] == '@') {
		if (selected_item[1] == 'b') {
			selected_item.erase(0, 3);
			selected_item.erase(selected_item.size()-1, 1);
		}
		else
		if (selected_item[1] == 'i')
			selected_item.erase(0, 4);
	}

#if defined(__linux__) || defined(__APPLE__)

	

	if (strcmp("/", path_obj->value()))
		selected_item.insert(0, "/");

	selected_item.insert(0, path_obj->value());
	return selected_item.c_str();
#elif defined(_WIN32)

	

	if (strcmp(path_obj->value(), "All drives") == 0)
			return selected_item.c_str();
	else {

		

		if (strlen(path_obj->value()) > 3) 
			selected_item.insert(0, "\\");

		selected_item.insert(0, path_obj->value());
		return selected_item.c_str();
	}
#endif
}





#ifdef _WIN32
void gBrowser::showDrives() {

	

	char drives[64];
	char *i = drives;		
	GetLogicalDriveStrings(64, drives);

	

	while (*i) {
		add(i);
		i = &i[strlen(i) + 1];
	}
}

#endif
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include <FL/fl_draw.H>
#include "ge_envelopeChannel.h"
#include "gd_actionEditor.h"
#include "gd_mainWindow.h"
#include "channel.h"
#include "recorder.h"
#include "mixer.h"


extern Mixer         G_Mixer;
extern gdMainWindow *mainWin;


gEnvelopeChannel::gEnvelopeChannel(int x, int y, gdActionEditor *pParent, int type, int range, const char *l)
	:	gActionWidget(x, y, 200, 80, pParent), l(l), type(type), range(range),
		selectedPoint(-1), draggedPoint(-1)
{
	size(pParent->totalWidth, h());
}





gEnvelopeChannel::~gEnvelopeChannel() {
	clearPoints();
}





void gEnvelopeChannel::addPoint(int frame, int iValue, float fValue, int px, int py) {
	point p;
	p.frame  = frame;
	p.iValue = iValue;
	p.fValue = fValue;
	p.x = px;
	p.y = py;
	points.add(p);
}





void gEnvelopeChannel::updateActions() {
	for (unsigned i=0; i<points.size; i++)
		points.at(i).x = points.at(i).frame / pParent->zoom;
}





void gEnvelopeChannel::draw() {

	baseDraw();

	

	fl_color(COLOR_BG_1);
	fl_font(FL_HELVETICA, 12);
	fl_draw(l, x()+4, y(), 80, h(), (Fl_Align) (FL_ALIGN_LEFT));

	int pxOld = x()-3;
	int pyOld = y()+1;
	int pxNew = 0;
	int pyNew = 0;

	fl_color(COLOR_BG_2);

	for (unsigned i=0; i<points.size; i++) {

		pxNew = points.at(i).x+x()-3;
		pyNew = points.at(i).y+y();

		if (selectedPoint == (int) i) {
			fl_color(COLOR_BD_1);
			fl_rectf(pxNew, pyNew, 7, 7);
			fl_color(COLOR_BG_2);
		}
		else
			fl_rectf(pxNew, pyNew, 7, 7);

		if (i > 0)
			fl_line(pxOld+3, pyOld+3, pxNew+3, pyNew+3);

		pxOld = pxNew;
		pyOld = pyNew;
	}
}





int gEnvelopeChannel::handle(int e) {

	

	int ret = 0;
	int mx  = Fl::event_x()-x();  
	int my  = Fl::event_y()-y();  

	switch (e) {

		case FL_ENTER: {
			ret = 1;
			break;
		}

		case FL_MOVE: {
			selectedPoint = getSelectedPoint();
			redraw();
			ret = 1;
			break;
		}

		case FL_LEAVE: {
			draggedPoint  = -1;
			selectedPoint = -1;
			redraw();
			ret = 1;
			break;
		}

		case FL_PUSH: {

			

			if (Fl::event_button1()) {

				if (selectedPoint != -1) {
					draggedPoint = selectedPoint;
				}
				else {

					

					if (my > h()-8) my = h()-8;
					if (mx > pParent->coverX-x()) mx = pParent->coverX-x();

					if (range == RANGE_FLOAT) {

						

						if (points.size == 0) {
							addPoint(0, 0, 1.0f, 0, 1);
							recorder::rec(pParent->chan->index, type, 0, 0, 1.0f);
							addPoint(G_Mixer.totalFrames, 0, 1.0f, pParent->coverX, 1);
							recorder::rec(pParent->chan->index, type, G_Mixer.totalFrames, 0, 1.0f);
						}

						

						int frame   = mx * pParent->zoom;
						float value = (my - h() + 8) / (float) (1 - h() + 8);
						addPoint(frame, 0, value, mx, my);
						recorder::rec(pParent->chan->index, type, frame, 0, value);
						recorder::sortActions();
						sortPoints();
					}
					else {
						
					}
					mainWin->keyboard->setChannelWithActions((gSampleChannel*)pParent->chan->guiChannel); 
					redraw();
				}
			}
			else {

				

				if (selectedPoint != -1) {
					if (selectedPoint == 0 || (unsigned) selectedPoint == points.size-1) {
						recorder::clearAction(pParent->chan->index, type);
						points.clear();
					}
					else {
						recorder::deleteAction(pParent->chan->index, points.at(selectedPoint).frame, type, false);
						recorder::sortActions();
						points.del(selectedPoint);
					}
					mainWin->keyboard->setChannelWithActions((gSampleChannel*)pParent->chan->guiChannel); 
					redraw();
				}
			}

			ret = 1;
			break;
		}

		case FL_RELEASE: {
			if (draggedPoint != -1) {

				if (points.at(draggedPoint).x == previousXPoint) {
					puts("nothing to do");
				}
				else {
					int newFrame = points.at(draggedPoint).x * pParent->zoom;

					

					if (newFrame < 0)
						newFrame = 0;
					else if (newFrame > G_Mixer.totalFrames)
						newFrame = G_Mixer.totalFrames;

					

					int vp = verticalPoint(points.at(draggedPoint));
					if (vp == 1) 			 newFrame -= 256;
					else if (vp == -1) newFrame += 256;

					

					recorder::deleteAction(pParent->chan->index,	points.at(draggedPoint).frame, type, false);

					if (range == RANGE_FLOAT) {
						float value = (points.at(draggedPoint).y - h() + 8) / (float) (1 - h() + 8);
						recorder::rec(pParent->chan->index, type, newFrame, 0, value);
					}
					else {
						
					}

					recorder::sortActions();
					points.at(draggedPoint).frame = newFrame;
					draggedPoint  = -1;
					selectedPoint = -1;
				}
			}
			ret = 1;
			break;
		}

		case FL_DRAG: {

			if (draggedPoint != -1) {

				

				if (my > h()-8)
					points.at(draggedPoint).y = h()-8;
				else
				if (my < 1)
					points.at(draggedPoint).y = 1;
				else
					points.at(draggedPoint).y = my;

				

				if (draggedPoint == 0)
					points.at(draggedPoint).x = x()-8;
				else
				if ((unsigned) draggedPoint == points.size-1)
					points.at(draggedPoint).x = pParent->coverX;
				else {
					int prevPoint = points.at(draggedPoint-1).x;
					int nextPoint = points.at(draggedPoint+1).x;
					if (mx <= prevPoint)
						points.at(draggedPoint).x = prevPoint;
					else
					if (mx >= nextPoint)
						points.at(draggedPoint).x = nextPoint;
					
					
					else {
						if (pParent->gridTool->isOn())
							points.at(draggedPoint).x = pParent->gridTool->getSnapPoint(mx)-1;
						else
							points.at(draggedPoint).x = mx;
					}
				}
				redraw();
			}

			ret = 1;
			break;
		}
	}

	return ret;
}





int gEnvelopeChannel::verticalPoint(const point &p) {
	for (unsigned i=0; i<points.size; i++) {
		if (&p == &points.at(i)) {
			if (i == 0 || i == points.size-1)  
				return 0;
			else {
				if (points.at(i-1).x == p.x)    
					return -1;
				else
				if (points.at(i+1).x == p.x)    
					return 1;
			}
			break;
		}
	}
	return 0;
}





void gEnvelopeChannel::sortPoints() {
	for (unsigned i=0; i<points.size; i++)
		for (unsigned j=0; j<points.size; j++)
			if (points.at(j).x > points.at(i).x)
				points.swap(j, i);
}





int gEnvelopeChannel::getSelectedPoint() {

	

	for (unsigned i=0; i<points.size; i++) {
		if (Fl::event_x() >= points.at(i).x+x()-4  &&
				Fl::event_x() <= points.at(i).x+x()+4  &&
				Fl::event_y() >= points.at(i).y+y()    &&
				Fl::event_y() <= points.at(i).y+y()+7)
		return i;
	}
	return -1;
}





void gEnvelopeChannel::fill() {
	points.clear();
	for (unsigned i=0; i<recorder::global.size; i++)
		for (unsigned j=0; j<recorder::global.at(i).size; j++) {
			recorder::action *a = recorder::global.at(i).at(j);
			if (a->type == type && a->chan == pParent->chan->index) {
				if (range == RANGE_FLOAT)
					addPoint(
						a->frame,                      
						0,                             
						a->fValue,                     
						a->frame / pParent->zoom,       
						((1-h()+8)*a->fValue)+h()-8);  
				
			}
		}

}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include <math.h>
#include "ge_mixed.h"
#include "gd_mainWindow.h"
#include "const.h"
#include "mixer.h"
#include "graphics.h"
#include "recorder.h"
#include "gui_utils.h"
#include "channel.h"
#include "sampleChannel.h"


extern Mixer 		     G_Mixer;
extern unsigned 		 G_beats;
extern bool  		     G_audio_status;
extern Patch 		     f_patch;
extern Conf  		     f_conf;
extern gdMainWindow *mainWin;


void __cb_window_closer(Fl_Widget *v, void *p) {
	delete (Fl_Window*)p;
}





gButton::gButton(int X, int Y, int W, int H, const char *L, const char **imgOff, const char **imgOn)
	: gClick(X, Y, W, H, L, imgOff, imgOn) {}





gStatus::gStatus(int x, int y, int w, int h, SampleChannel *ch, const char *L)
: Fl_Box(x, y, w, h, L), ch(ch) {}

void gStatus::draw() {

	fl_rect(x(), y(), w(), h(), COLOR_BD_0);		          
	fl_rectf(x()+1, y()+1, w()-2, h()-2, COLOR_BG_0);		  

	if (ch != NULL) {
		if (ch->status    & (STATUS_WAIT | STATUS_ENDING | REC_ENDING | REC_WAITING) ||
				ch->recStatus & (REC_WAITING | REC_ENDING))
		{
			fl_rect(x(), y(), w(), h(), COLOR_BD_1);
		}
		else
		if (ch->status == STATUS_PLAY)
			fl_rect(x(), y(), w(), h(), COLOR_BD_1);
		else
			fl_rectf(x()+1, y()+1, w()-2, h()-2, COLOR_BG_0);     


		if (G_Mixer.chanInput == ch)
			fl_rectf(x()+1, y()+1, w()-2, h()-2, COLOR_BG_3);	    
		else
		if (recorder::active && recorder::canRec(ch))
			fl_rectf(x()+1, y()+1, w()-2, h()-2, COLOR_BG_4);     

		

		int pos = ch->getPosition();
		if (pos == -1)
			pos = 0;
		else
			pos = (pos * (w()-1)) / (ch->end - ch->begin);
		fl_rectf(x()+1, y()+1, pos, h()-2, COLOR_BG_2);
	}
}





gClick::gClick(int x, int y, int w, int h, const char *L, const char **imgOff, const char **imgOn)
: Fl_Button(x, y, w, h, L),
	imgOff(imgOff),
	imgOn(imgOn),
	bgColor0(COLOR_BG_0),
	bgColor1(COLOR_BG_1),
	bdColor(COLOR_BD_0),
	txtColor(COLOR_TEXT_0)	{}

void gClick::draw() {

	if (!active()) txtColor = bdColor;
	else 					 txtColor = COLOR_TEXT_0;

	fl_rect(x(), y(), w(), h(), bdColor);             
	if (value()) {													          
		if (imgOn != NULL)
			fl_draw_pixmap(imgOn, x()+1, y()+1);
		else
			fl_rectf(x(), y(), w(), h(), bgColor1);       
	}
	else {                                            
		fl_rectf(x()+1, y()+1, w()-2, h()-2, bgColor0); 
		if (imgOff != NULL)
			fl_draw_pixmap(imgOff, x()+1, y()+1);
	}
	if (!active())
		fl_color(FL_INACTIVE_COLOR);

	fl_color(txtColor);
	fl_font(FL_HELVETICA, 11);
	fl_draw(label(), x(), y(), w(), h(), FL_ALIGN_CENTER);
}





gClickRepeat::gClickRepeat(int x, int y, int w, int h, const char *L, const char **imgOff, const char **imgOn)
: Fl_Repeat_Button(x, y, w, h, L), imgOff(imgOff), imgOn(imgOn) {}

void gClickRepeat::draw() {
	if (value()) {															 
		fl_rectf(x(), y(), w(), h(), COLOR_BG_1);  
		if (imgOn != NULL)
			fl_draw_pixmap(imgOn, x()+1, y()+1);
	}
	else {                                       
		fl_rectf(x(), y(), w(), h(), COLOR_BG_0);  
		fl_rect(x(), y(), w(), h(), COLOR_BD_0);   
		if (imgOff != NULL)
			fl_draw_pixmap(imgOff, x()+1, y()+1);
	}
	if (!active())
		fl_color(FL_INACTIVE_COLOR);

	fl_color(COLOR_TEXT_0);
	fl_font(FL_HELVETICA, 11);
	fl_draw(label(), x(), y(), w(), h(), FL_ALIGN_CENTER);
}





gInput::gInput(int x, int y, int w, int h, const char *L)
: Fl_Input(x, y, w, h, L) {
	
	box(G_BOX);
	labelsize(11);
	labelcolor(COLOR_TEXT_0);
	color(COLOR_BG_DARK);
	textcolor(COLOR_TEXT_0);
	cursor_color(COLOR_TEXT_0);
	selection_color(COLOR_BD_0);
	textsize(11);

}





gDial::gDial(int x, int y, int w, int h, const char *L)
: Fl_Dial(x, y, w, h, L) {
	labelsize(11);
	labelcolor(COLOR_TEXT_0);
	align(FL_ALIGN_LEFT);
	type(FL_FILL_DIAL);
	angles(0, 360);
	color(COLOR_BG_0);            
	selection_color(COLOR_BG_1);   
}

void gDial::draw() {
	double angle = (angle2()-angle1())*(value()-minimum())/(maximum()-minimum()) + angle1();

	fl_color(COLOR_BG_0);
	fl_pie(x(), y(), w(), h(), 270-angle1(), angle > angle1() ? 360+270-angle : 270-360-angle);

	fl_color(COLOR_BD_0);
	fl_arc(x(), y(), w(), h(), 0, 360);
	fl_pie(x(), y(), w(), h(), 270-angle, 270-angle1());
}




gBox::gBox(int x, int y, int w, int h, const char *L, Fl_Align al)
: Fl_Box(x, y, w, h, L) {
	labelsize(11);
	box(FL_NO_BOX);
	labelcolor(COLOR_TEXT_0);
	if (al != 0)
		align(al | FL_ALIGN_INSIDE);
}





gCheck::gCheck(int x, int y, int w, int h, const char *L)
: Fl_Check_Button(x, y, w, h, L) {}

void gCheck::draw() {

	int color = !active() ? FL_INACTIVE_COLOR : COLOR_BD_0;

	if (value()) {
		fl_rect(x(), y(), 12, 12, (Fl_Color) color);
		fl_rectf(x(), y(), 12, 12, (Fl_Color) color);
	}
	else {
		fl_rectf(x(), y(), 12, 12, FL_BACKGROUND_COLOR);
		fl_rect(x(), y(), 12, 12, (Fl_Color) color);
	}

	fl_rectf(x()+20, y(), w(), h(), FL_BACKGROUND_COLOR);  
	fl_font(FL_HELVETICA, 11);
	fl_color(COLOR_TEXT_0);
	fl_draw(label(), x()+20, y(), w(), h(), (Fl_Align) (FL_ALIGN_LEFT | FL_ALIGN_TOP));
}





gRadio::gRadio(int x, int y, int w, int h, const char *L)
: Fl_Radio_Button(x, y, w, h, L) {}

void gRadio::draw() {

	int color = !active() ? FL_INACTIVE_COLOR : COLOR_BD_0;

	if (value()) {
		fl_rect(x(), y(), 12, 12, (Fl_Color) color);
		fl_rectf(x(), y(), 12, 12, (Fl_Color) color);
	}
	else {
		fl_rectf(x(), y(), 12, 12, FL_BACKGROUND_COLOR);
		fl_rect(x(), y(), 12, 12, (Fl_Color) color);
	}

	fl_rectf(x()+20, y(), w(), h(), FL_BACKGROUND_COLOR);  
	fl_font(FL_HELVETICA, 11);
	fl_color(COLOR_TEXT_0);
	fl_draw(label(), x()+20, y(), w(), h(), (Fl_Align) (FL_ALIGN_LEFT | FL_ALIGN_TOP));
}





gProgress::gProgress(int x, int y, int w, int h, const char *L)
: Fl_Progress(x, y, w, h, L) {
	color(COLOR_BG_0, COLOR_BD_0);
	box(G_BOX);

}





gSoundMeter::gSoundMeter(int x, int y, int w, int h, const char *L)
	: Fl_Box(x, y, w, h, L),
		clip(false),
		mixerPeak(0.0f),
		peak(0.0f),
		peak_old(0.0f),
		db_level(0.0f),
		db_level_old(0.0f) {}

void gSoundMeter::draw() {

	fl_rect(x(), y(), w(), h(), COLOR_BD_0);

	

	peak = 0.0f;
	float tmp_peak = 0.0f;

	tmp_peak = fabs(mixerPeak);
	if (tmp_peak > peak)
		peak = tmp_peak;

	clip = peak >= 1.0f ? true : false; 


	

	db_level = 20 * log10(peak);
	if (db_level < db_level_old)
		if (db_level_old > -DB_MIN_SCALE)
			db_level = db_level_old - 2.0f;

	db_level_old = db_level;

	

	float px_level = 0.0f;
	if (db_level < 0.0f)
		px_level = ((w()/DB_MIN_SCALE) * db_level) + w();
	else
		px_level = w();

	fl_rectf(x()+1, y()+1, w()-2, h()-2, COLOR_BG_0);
	fl_rectf(x()+1, y()+1, (int) px_level, h()-2, clip || !G_audio_status ? COLOR_ALERT : COLOR_BD_0);
}



gBeatMeter::gBeatMeter(int x, int y, int w, int h, const char *L)
	: Fl_Box(x, y, w, h, L) {}

void gBeatMeter::draw() {

	int cursorW = (w()/MAX_BEATS);

	fl_rect(x(), y(), w(), h(), COLOR_BD_0);													      
	fl_rectf(x()+1, y()+1, w()-2, h()-2, FL_BACKGROUND_COLOR);  						
	fl_rectf(x()+(G_Mixer.actualBeat*cursorW)+3, y()+3, cursorW-5, h()-6, COLOR_BG_2); 

	

	fl_color(COLOR_BD_0);
	for (int i=1; i<=G_Mixer.beats; i++)
		fl_line(x()+cursorW*i, y()+1, x()+cursorW*i, y()+h()-2);

	

	fl_color(COLOR_BG_2);
	int delta = G_Mixer.beats / G_Mixer.bars;
	for (int i=1; i<G_Mixer.bars; i++)
		fl_line(x()+cursorW*(i*delta), y()+1, x()+cursorW*(i*delta), y()+h()-2);

	

	fl_rectf(x()+(G_Mixer.beats*cursorW)+1, y()+1, ((MAX_BEATS-G_Mixer.beats)*cursorW)-1, h()-2, COLOR_BG_1);
}







gModeBox::gModeBox(int x, int y, int w, int h, SampleChannel *ch, const char *L)
	: Fl_Menu_Button(x, y, w, h, L), ch(ch)
{
	box(G_BOX);
	textsize(11);
	textcolor(COLOR_TEXT_0);
	color(COLOR_BG_0);

	add("Loop . basic", 	   0, cb_change_chanmode, (void *)LOOP_BASIC);
	add("Loop . once", 		   0, cb_change_chanmode, (void *)LOOP_ONCE);
	add("Loop . repeat", 	   0, cb_change_chanmode, (void *)LOOP_REPEAT);
	add("Oneshot . basic",   0, cb_change_chanmode, (void *)SINGLE_BASIC);
	add("Oneshot . press",   0, cb_change_chanmode, (void *)SINGLE_PRESS);
	add("Oneshot . retrig",  0, cb_change_chanmode, (void *)SINGLE_RETRIG);
	add("Oneshot . endless", 0, cb_change_chanmode, (void *)SINGLE_ENDLESS);
}


void gModeBox::draw() {
	fl_rect(x(), y(), w(), h(), COLOR_BD_0);		
	switch (ch->mode) {
		case LOOP_BASIC:
			fl_draw_pixmap(loopBasic_xpm, x()+1, y()+1);
			break;
		case LOOP_ONCE:
			fl_draw_pixmap(loopOnce_xpm, x()+1, y()+1);
			break;
		case LOOP_REPEAT:
			fl_draw_pixmap(loopRepeat_xpm, x()+1, y()+1);
			break;
		case SINGLE_BASIC:
			fl_draw_pixmap(oneshotBasic_xpm, x()+1, y()+1);
			break;
		case SINGLE_PRESS:
			fl_draw_pixmap(oneshotPress_xpm, x()+1, y()+1);
			break;
		case SINGLE_RETRIG:
			fl_draw_pixmap(oneshotRetrig_xpm, x()+1, y()+1);
			break;
		case SINGLE_ENDLESS:
			fl_draw_pixmap(oneshotEndless_xpm, x()+1, y()+1);
			break;
	}
}


void gModeBox::cb_change_chanmode(Fl_Widget *v, void *p) { ((gModeBox*)v)->__cb_change_chanmode((intptr_t)p); }


void gModeBox::__cb_change_chanmode(int mode) {
	ch->mode = mode;

	

	gu_refreshActionEditor();
}





gChoice::gChoice(int x, int y, int w, int h, const char *l, bool ang)
	: Fl_Choice(x, y, w, h, l), angle(ang) {
	labelsize(11);
	labelcolor(COLOR_TEXT_0);
	box(FL_BORDER_BOX);
	textsize(11);
	textcolor(COLOR_TEXT_0);
	color(COLOR_BG_0);
}


void gChoice::draw() {
	fl_rectf(x(), y(), w(), h(), COLOR_BG_0);              
	fl_rect(x(), y(), w(), h(), (Fl_Color) COLOR_BD_0);    
	if (angle)
		fl_polygon(x()+w()-8, y()+h()-1, x()+w()-1, y()+h()-8, x()+w()-1, y()+h()-1);

	

	fl_color(!active() ? COLOR_BD_0 : COLOR_TEXT_0);
	if (value() != -1) {
		if (fl_width(text(value())) < w()-8) {
			fl_draw(text(value()), x(), y(), w(), h(), FL_ALIGN_CENTER);
		}
		else {
			std::string tmp = text(value());
			int size        = tmp.size();
			while (fl_width(tmp.c_str()) >= w()-16) {
				tmp.resize(size);
				size--;
			}
			tmp += "...";
			fl_draw(tmp.c_str(), x(), y(), w(), h(), FL_ALIGN_CENTER);
		}

	}
}





void gDrawBox(int x, int y, int w, int h, Fl_Color c) {
	fl_color(c);
  fl_rectf(x, y, w, h);
  fl_color(COLOR_BD_0);
  fl_rect(x, y, w, h);
}





gLiquidScroll::gLiquidScroll(int x, int y, int w, int h, const char *l)
	: Fl_Scroll(x, y, w, h, l)
{
	type(Fl_Scroll::VERTICAL);
	scrollbar.color(COLOR_BG_0);
	scrollbar.selection_color(COLOR_BG_1);
	scrollbar.labelcolor(COLOR_BD_1);
	scrollbar.slider(G_BOX);
}


void gLiquidScroll::resize(int X, int Y, int W, int H) {
	int nc = children()-2;                
	for ( int t=0; t<nc; t++) {					  
		Fl_Widget *c = child(t);
		c->resize(c->x(), c->y(), W-24, c->h());    
	}
	init_sizes();		
	Fl_Scroll::resize(X,Y,W,H);
}





gSlider::gSlider(int x, int y, int w, int h, const char *l)
	: Fl_Slider(x, y, w, h, l)
{
	type(FL_HOR_FILL_SLIDER);

	labelsize(11);
	align(FL_ALIGN_LEFT);
	labelcolor(COLOR_TEXT_0);

	box(G_BOX);
	color(COLOR_BG_0);
	selection_color(COLOR_BD_0);
}





gResizerBar::gResizerBar(int X,int Y,int W,int H)
	: Fl_Box(X,Y,W,H)
{
	orig_h = H;
	last_y = 0;
	min_h  = 30;
	align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
	labelfont(FL_COURIER);
	labelsize(H);
	visible_focus(0);
	box(FL_FLAT_BOX);
}


void gResizerBar::HandleDrag(int diff) {
	Fl_Scroll *grp = (Fl_Scroll*)parent();
	int top = y();
	int bot = y()+h();

	
	

	for (int t=0; t<grp->children(); t++) {
		Fl_Widget *w = grp->child(t);
		if ((w->y()+w->h()) == top) {                           
			if ((w->h()+diff) < min_h) diff = w->h() - min_h;     
			w->resize(w->x(), w->y(), w->w(), w->h()+diff);       
			break;                                                
		}
	}

	

	for (int t=0; t<grp->children(); t++) {
		Fl_Widget *w = grp->child(t);
		if (w->y() >= bot)                                     
			w->resize(w->x(), w->y()+diff, w->w(), w->h());      
	}

	

	resize(x(),y()+diff,w(),h());
	grp->init_sizes();
	grp->redraw();
}


void gResizerBar::SetMinHeight(int val) { min_h = val; }
int  gResizerBar::GetMinHeight() { return min_h; }


int gResizerBar::handle(int e) {
	int ret = 0;
	int this_y = Fl::event_y_root();
	switch (e) {
		case FL_FOCUS: ret = 1; break;
		case FL_ENTER: ret = 1; fl_cursor(FL_CURSOR_NS);      break;
		case FL_LEAVE: ret = 1; fl_cursor(FL_CURSOR_DEFAULT); break;
		case FL_PUSH:  ret = 1; last_y = this_y;              break;
		case FL_DRAG:
			HandleDrag(this_y-last_y);
			last_y = this_y;
			ret = 1;
			break;
		default: break;
	}
	return(Fl_Box::handle(e) | ret);
}


void gResizerBar::resize(int X,int Y,int W,int H) {
	Fl_Box::resize(X,Y,W,orig_h);                                
}





gScroll::gScroll(int x, int y, int w, int h, int t)
	: Fl_Scroll(x, y, w, h)
{
	type(t);

	scrollbar.color(COLOR_BG_0);
	scrollbar.selection_color(COLOR_BG_1);
	scrollbar.labelcolor(COLOR_BD_1);
	scrollbar.slider(G_BOX);

	hscrollbar.color(COLOR_BG_0);
	hscrollbar.selection_color(COLOR_BG_1);
	hscrollbar.labelcolor(COLOR_BD_1);
	hscrollbar.slider(G_BOX);
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "ge_muteChannel.h"
#include "gd_actionEditor.h"
#include "ge_actionWidget.h"
#include "gd_mainWindow.h"
#include "recorder.h"
#include "mixer.h"
#include "glue.h"
#include "channel.h"


extern gdMainWindow *mainWin;
extern Mixer         G_Mixer;


gMuteChannel::gMuteChannel(int x, int y, gdActionEditor *pParent)
 : gActionWidget(x, y, 200, 80, pParent), draggedPoint(-1), selectedPoint(-1)
{
	size(pParent->totalWidth, h());
	extractPoints();
}





void gMuteChannel::draw() {

	baseDraw();

	

	fl_color(COLOR_BG_1);
	fl_font(FL_HELVETICA, 12);
	fl_draw("mute", x()+4, y(), w(), h(), (Fl_Align) (FL_ALIGN_LEFT | FL_ALIGN_CENTER));

	

	fl_color(COLOR_BG_1);
	fl_font(FL_HELVETICA, 9);
	fl_draw("on",  x()+4, y(),        w(), h(), (Fl_Align) (FL_ALIGN_LEFT | FL_ALIGN_TOP));
	fl_draw("off", x()+4, y()+h()-14, w(), h(), (Fl_Align) (FL_ALIGN_LEFT | FL_ALIGN_TOP));

	

	fl_color(COLOR_BG_2);

	int pxOld = x()+1;
	int pxNew = 0;
	int py    = y()+h()-5;
	int pyDot = py-6;

	for (unsigned i=0; i<points.size; i++) {

		

		pxNew = points.at(i).x+x();

		

		fl_line(pxOld, py, pxNew, py);
		pxOld = pxNew;

		py = i % 2 == 0 ? y()+4 : y()+h()-5;

		

		fl_line(pxNew, y()+h()-5, pxNew, y()+4);

		if (selectedPoint == (int) i) {
			fl_color(COLOR_BD_1);
			fl_rectf(pxNew-3, pyDot, 7, 7);
			fl_color(COLOR_BG_2);
		}
		else
			fl_rectf(pxNew-3, pyDot, 7, 7);
	}

	

	py = y()+h()-5;
	fl_line(pxNew+3, py, pParent->coverX+x()-1, py);
}





void gMuteChannel::extractPoints() {

	points.clear();

	

	for (unsigned i=0; i<recorder::frames.size; i++) {
		for (unsigned j=0; j<recorder::global.at(i).size; j++) {
			if (recorder::global.at(i).at(j)->chan == pParent->chan->index) {
				if (recorder::global.at(i).at(j)->type & (ACTION_MUTEON | ACTION_MUTEOFF)) {
					point p;
					p.frame = recorder::frames.at(i);
					p.type  = recorder::global.at(i).at(j)->type;
					p.x     = p.frame / pParent->zoom;
					points.add(p);
					
				}
			}
		}
	}
}





void gMuteChannel::updateActions() {
	for (unsigned i=0; i<points.size; i++)
		points.at(i).x = points.at(i).frame / pParent->zoom;
}





int gMuteChannel::handle(int e) {

	int ret = 0;
	int mouseX = Fl::event_x()-x();

	switch (e) {

		case FL_ENTER: {
			ret = 1;
			break;
		}

		case FL_MOVE: {
			selectedPoint = getSelectedPoint();
			redraw();
			ret = 1;
			break;
		}

		case FL_LEAVE: {
			draggedPoint  = -1;
			selectedPoint = -1;
			redraw();
			ret = 1;
			break;
		}

		case FL_PUSH: {

			

			if (Fl::event_button1())  {

				if (selectedPoint != -1) {
					draggedPoint   = selectedPoint;
					previousXPoint = points.at(selectedPoint).x;
				}
				else {

					

					if (mouseX > pParent->coverX) {
						ret = 1;
						break;
					}

					

					unsigned nextPoint = points.size;
					for (unsigned i=0; i<points.size; i++) {
						if (mouseX < points.at(i).x) {
							nextPoint = i;
							break;
						}
					}

					

					int frame_a = mouseX * pParent->zoom;
					int frame_b = frame_a+2048;

					if (pParent->gridTool->isOn()) {
						frame_a = pParent->gridTool->getSnapFrame(mouseX);
						frame_b = pParent->gridTool->getSnapFrame(mouseX + pParent->gridTool->getCellSize());

						

						if (pointCollides(frame_a) || pointCollides(frame_b)) {
							ret = 1;
							break;
						}
					}

					

					if (frame_a % 2 != 0) frame_a++;
					if (frame_b % 2 != 0) frame_b++;

					

					if (frame_b >= G_Mixer.totalFrames) {
						frame_b = G_Mixer.totalFrames;
						frame_a = frame_b-2048;
					}

					if (nextPoint % 2 != 0) {
						recorder::rec(pParent->chan->index, ACTION_MUTEOFF, frame_a);
						recorder::rec(pParent->chan->index, ACTION_MUTEON,  frame_b);
					}
					else {
						recorder::rec(pParent->chan->index, ACTION_MUTEON,  frame_a);
						recorder::rec(pParent->chan->index, ACTION_MUTEOFF, frame_b);
					}
					recorder::sortActions();

					mainWin->keyboard->setChannelWithActions((gSampleChannel*)pParent->chan->guiChannel); 
					extractPoints();
					redraw();
				}
			}
			else {

				

				if (selectedPoint != -1) {

					unsigned a;
					unsigned b;

					if (points.at(selectedPoint).type == ACTION_MUTEOFF) {
						a = selectedPoint-1;
						b = selectedPoint;
					}
					else {
						a = selectedPoint;
						b = selectedPoint+1;
					}

					
					

					recorder::deleteAction(pParent->chan->index, points.at(a).frame,	points.at(a).type, false); 
					recorder::deleteAction(pParent->chan->index,	points.at(b).frame,	points.at(b).type, false); 
					recorder::sortActions();

					mainWin->keyboard->setChannelWithActions((gSampleChannel*)pParent->chan->guiChannel); 
					extractPoints();
					redraw();
				}
			}
			ret = 1;
			break;
		}

		case FL_RELEASE: {

			if (draggedPoint != -1) {

				if (points.at(draggedPoint).x == previousXPoint) {
					
				}
				else {

					int newFrame = points.at(draggedPoint).x * pParent->zoom;

					recorder::deleteAction(
							pParent->chan->index,
							points.at(draggedPoint).frame,
							points.at(draggedPoint).type,
							false);  

					recorder::rec(
							pParent->chan->index,
							points.at(draggedPoint).type,
							newFrame);

					recorder::sortActions();

					points.at(draggedPoint).frame = newFrame;
				}
			}
			draggedPoint  = -1;
			selectedPoint = -1;

			ret = 1;
			break;
		}

		case FL_DRAG: {

			if (draggedPoint != -1) {

				

				int prevPoint;
				int nextPoint;

				if (draggedPoint == 0) {
					prevPoint = 0;
					nextPoint = points.at(draggedPoint+1).x - 1;
					if (pParent->gridTool->isOn())
						nextPoint -= pParent->gridTool->getCellSize();
				}
				else
				if ((unsigned) draggedPoint == points.size-1) {
					prevPoint = points.at(draggedPoint-1).x + 1;
					nextPoint = pParent->coverX-x();
					if (pParent->gridTool->isOn())
						prevPoint += pParent->gridTool->getCellSize();
				}
				else {
					prevPoint = points.at(draggedPoint-1).x + 1;
					nextPoint = points.at(draggedPoint+1).x - 1;
					if (pParent->gridTool->isOn()) {
						prevPoint += pParent->gridTool->getCellSize();
						nextPoint -= pParent->gridTool->getCellSize();
					}
				}

				if (mouseX <= prevPoint)
					points.at(draggedPoint).x = prevPoint;
				else
				if (mouseX >= nextPoint)
					points.at(draggedPoint).x = nextPoint;
				else
				if (pParent->gridTool->isOn())
					points.at(draggedPoint).x = pParent->gridTool->getSnapPoint(mouseX)-1;
				else
					points.at(draggedPoint).x = mouseX;

				redraw();
			}
			ret = 1;
			break;
		}
	}


	return ret;
}





bool gMuteChannel::pointCollides(int frame) {
	for (unsigned i=0; i<points.size; i++)
		if (frame == points.at(i).frame)
			return true;
	return false;
}





int gMuteChannel::getSelectedPoint() {

	

	for (unsigned i=0; i<points.size; i++) {
		if (Fl::event_x() >= points.at(i).x+x()-3 &&
				Fl::event_x() <= points.at(i).x+x()+3)
		return i;
	}
	return -1;
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include <FL/fl_draw.H>
#include "ge_pianoRoll.h"
#include "gd_mainWindow.h"
#include "gd_actionEditor.h"
#include "channel.h"
#include "midiChannel.h"
#include "const.h"
#include "kernelMidi.h"


extern gdMainWindow *mainWin;
extern Mixer         G_Mixer;
extern Conf	         G_Conf;


gPianoRollContainer::gPianoRollContainer(int x, int y, class gdActionEditor *pParent)
 : Fl_Scroll(x, y, 200, 422), pParent(pParent)
{
	size(pParent->totalWidth, G_Conf.pianoRollH);
	pianoRoll = new gPianoRoll(x, y, pParent->totalWidth, pParent);
}





gPianoRollContainer::~gPianoRollContainer() {
	clear();
	G_Conf.pianoRollH = h();
	G_Conf.pianoRollY = pianoRoll->y();
}





void gPianoRollContainer::updateActions() {
	pianoRoll->updateActions();
}





void gPianoRollContainer::draw() {

	pianoRoll->size(this->w(), pianoRoll->h());  

	

	fl_rectf(x(), y(), w(), h(), COLOR_BG_MAIN);

	

	fl_push_clip(x(), y(), w(), h());
	draw_child(*pianoRoll);
	fl_pop_clip();

	fl_color(COLOR_BD_0);
	fl_line_style(0);
	fl_rect(x(), y(), pParent->totalWidth, h());
}







gPianoRoll::gPianoRoll(int X, int Y, int W, class gdActionEditor *pParent)
 : gActionWidget(X, Y, W, 40, pParent)
{
	resizable(NULL);                      
	size(W, (MAX_NOTES+1) * CELL_H);      

	if (G_Conf.pianoRollY == -1)
		position(x(), y()-(h()/2));  
	else
		position(x(), G_Conf.pianoRollY);

	drawSurface1();
	drawSurface2();

	

	recorder::sortActions();

	recorder::action *a2   = NULL;
	recorder::action *prev = NULL;

	for (unsigned i=0; i<recorder::frames.size; i++) {
		for (unsigned j=0; j<recorder::global.at(i).size; j++) {

			
			

			if (recorder::frames.at(i) > G_Mixer.totalFrames)
				continue;

			recorder::action *a1 = recorder::global.at(i).at(j);

			if (a1->chan != pParent->chan->index)
				continue;

			if (a1->type == ACTION_MIDI) {

				

				if (a1 == prev) {
					
					continue;
				}

				

				int a1_type = kernelMidi::getB1(a1->iValue);
				int a1_note = kernelMidi::getB2(a1->iValue);
				int a1_velo = kernelMidi::getB3(a1->iValue);

				if (a1_type == 0x80) {
					
					continue;
				}

				

				recorder::getNextAction(
						a1->chan,
						ACTION_MIDI,
						a1->frame,
						&a2,
						kernelMidi::getIValue(0x80, a1_note, a1_velo));

				

				if (a2) {
					
					
					
					new gPianoItem(0, 0, x(), y()+3, a1, a2, pParent);
					prev = a2;
					a2 = NULL;
				}
				else
					printf("[gPianoRoll] recorder didn't find action!\n");

			}
		}
	}

	end();
}





void gPianoRoll::drawSurface1() {

	surface1 = fl_create_offscreen(40, h());
	fl_begin_offscreen(surface1);

	

	fl_rectf(0, 0, 40, h(), COLOR_BG_MAIN);

	fl_line_style(FL_DASH, 0, NULL);
	fl_font(FL_HELVETICA, 11);

	int octave = 9;

	for (int i=1; i<=MAX_NOTES+1; i++) {

		

		char note[6];
		int  step = i % 12;

		switch (step) {
			case 1:
				fl_rectf(0, i*CELL_H, 40, CELL_H, 30, 30, 30);
				sprintf(note, "%dG", octave);
				break;
			case 2:
				sprintf(note, "%dF#", octave);
				break;
			case 3:
				sprintf(note, "%dF", octave);
				break;
			case 4:
				fl_rectf(0, i*CELL_H, 40, CELL_H, 30, 30, 30);
				sprintf(note, "%dE", octave);
				break;
			case 5:
				sprintf(note, "%dD#", octave);
				break;
			case 6:
				fl_rectf(0, i*CELL_H, 40, CELL_H, 30, 30, 30);
				sprintf(note, "%dD", octave);
				break;
			case 7:
				sprintf(note, "%dC#", octave);
				break;
			case 8:
				sprintf(note, "%dC", octave);
				break;
			case 9:
				fl_rectf(0, i*CELL_H, 40, CELL_H, 30, 30, 30);
				sprintf(note, "%dB", octave);
				break;
			case 10:
				sprintf(note, "%dA#", octave);
				break;
			case 11:
				fl_rectf(0, i*CELL_H, 40, CELL_H, 30, 30, 30);
				sprintf(note, "%dA", octave);
				break;
			case 0:
				sprintf(note, "%dG#", octave);
				octave--;
				break;
		}

		fl_color(fl_rgb_color(54, 54, 54));
		fl_draw(note, 4, ((i-1)*CELL_H)+1, 30, CELL_H, (Fl_Align) (FL_ALIGN_LEFT | FL_ALIGN_CENTER));

		

		if (i < 128)
			fl_line(0, i*CELL_H, 40, +i*CELL_H);
	}

	fl_line_style(0);
	fl_end_offscreen();
}





void gPianoRoll::drawSurface2() {
	surface2 = fl_create_offscreen(40, h());
	fl_begin_offscreen(surface2);
	fl_rectf(0, 0, 40, h(), COLOR_BG_MAIN);
	fl_color(fl_rgb_color(54, 54, 54));
	fl_line_style(FL_DASH, 0, NULL);
	for (int i=1; i<=MAX_NOTES+1; i++) {
		int  step = i % 12;
		switch (step) {
			case 1:
			case 4:
			case 6:
			case 9:
			case 11:
				fl_rectf(0, i*CELL_H, 40, CELL_H, 30, 30, 30);
				break;
		}
		if (i < 128) {
			fl_color(fl_rgb_color(54, 54, 54));
			fl_line(0, i*CELL_H, 40, +i*CELL_H);
		}
	}
	fl_line_style(0);
	fl_end_offscreen();
}





void gPianoRoll::draw() {

	fl_copy_offscreen(x(), y(), 40, h(), surface1, 0, 0);

#if defined(__APPLE__)
	for (int i=36; i<pParent->totalWidth; i+=36) 
		fl_copy_offscreen(x()+i, y(), 40, h(), surface2, 1, 0);
#else
	for (int i=40; i<pParent->totalWidth; i+=40) 
		fl_copy_offscreen(x()+i, y(), 40, h(), surface2, 0, 0);
#endif

	baseDraw(false);
	draw_children();
}





int gPianoRoll::handle(int e) {

	int ret = Fl_Group::handle(e);

	switch (e) {
		case FL_PUSH:	{

			

			if (Fl::event_x() >= pParent->coverX) {
				ret = 1;
				break;
			}


			push_y = Fl::event_y() - y();

			if (Fl::event_button1()) {

				

				int ax = Fl::event_x();
				int ay = Fl::event_y();

				

				int edge = (ay-y()-3) % 15;
				if (edge != 0) ay -= edge;

				

				if (!onItem(ax, ay-y()-3)) {
					int greyover = ax+20 - pParent->coverX-x();
					if (greyover > 0)
						ax -= greyover;
					add(new gPianoItem(ax, ay, ax-x(), ay-y()-3, NULL, NULL, pParent));
					redraw();
				}
			}
			ret = 1;
			break;
		}
		case FL_DRAG:	{

			if (Fl::event_button3()) {

				gPianoRollContainer *prc = (gPianoRollContainer*) parent();
				position(x(), Fl::event_y() - push_y);

				if (y() > prc->y())
					position(x(), prc->y());
				else
				if (y() < prc->y()+prc->h()-h())
					position(x(), prc->y()+prc->h()-h());

				prc->redraw();
			}
			ret = 1;
			break;
		}
		case FL_MOUSEWHEEL: {   
			ret = 1;
			break;
		}
	}
	return ret;
}





void gPianoRoll::updateActions() {

	

	gPianoItem *i;
	for (int k=0; k<children(); k++) {
		i = (gPianoItem*) child(k);

		

		int newX = x() + (i->getFrame_a() / pParent->zoom);
		int newW = ((i->getFrame_b() - i->getFrame_a()) / pParent->zoom);
		if (newW < 8)
			newW = 8;
		i->resize(newX, i->y(), newW, i->h());
		i->redraw();

		
	}
}





bool gPianoRoll::onItem(int rel_x, int rel_y) {

	if (!pParent->chan->hasActions)
		return false;

	int note = MAX_NOTES - (rel_y / CELL_H);

	int n = children();
	for (int i=0; i<n; i++) {   

		gPianoItem *p = (gPianoItem*) child(i);
		if (p->getNote() != note)
			continue;

		

		int start = p->x() > rel_x ? p->x() : rel_x-1;
		int end   = p->x()+p->w() < rel_x + 20 ? p->x()+p->w() : rel_x + 21;
		if (start < end)
			return true;
	}
	return false;
}






gPianoItem::gPianoItem(int X, int Y, int rel_x, int rel_y, recorder::action *a, recorder::action *b, gdActionEditor *pParent)
	: Fl_Box  (X, Y, 20, gPianoRoll::CELL_H-5),
	  a       (a),
	  b       (b),
		pParent (pParent),
		selected(false),
		event_a (0x00),
		event_b (0x00),
		changed (false)
{

	

	if (a) {
		note    = kernelMidi::getB2(a->iValue);
		frame_a = a->frame;
		frame_b = b->frame;
		event_a = a->iValue;
		event_b = b->iValue;
		int newX = rel_x + (frame_a / pParent->zoom);
		int newY = rel_y + getY(note);
		int newW = (frame_b - frame_a) / pParent->zoom;
		resize(newX, newY, newW, h());
	}

	

	else {
		note    = getNote(rel_y);
		frame_a = rel_x * pParent->zoom;
		frame_b = (rel_x + 20) * pParent->zoom;
		record();
		size((frame_b - frame_a) / pParent->zoom, h());
	}
}





bool gPianoItem::overlap() {

	

	gPianoRoll *pPiano = (gPianoRoll*) parent();

	for (int i=0; i<pPiano->children(); i++) {

		gPianoItem *pItem = (gPianoItem*) pPiano->child(i);

		

		if (pItem == this || pItem->y() != y())
			continue;

		int start = pItem->x() >= x() ? pItem->x() : x();
		int end   = pItem->x()+pItem->w() < x()+w() ? pItem->x()+pItem->w() : x()+w();
		if (start < end)
			return true;
	}

	return false;
}





void gPianoItem::draw() {
	int _w = w() > 4 ? w() : 4;
	
	fl_rectf(x(), y(), _w, h(), (Fl_Color) selected ? COLOR_BD_1 : COLOR_BG_2);
}





void gPianoItem::record() {

	

	int overflow = frame_b - G_Mixer.totalFrames;
	if (overflow > 0) {
		frame_b -= overflow;
		frame_a -= overflow;
	}

	
	
	event_a |= (0x90 << 24);   
	event_a |= (note << 16);   
	event_a |= (0x3F <<  8);   
	event_a |= (0x00);

	event_b |= (0x80 << 24);   
	event_b |= (note << 16);   
	event_b |= (0x3F <<  8);   
	event_b |= (0x00);

	recorder::rec(pParent->chan->index, ACTION_MIDI, frame_a, event_a);
	recorder::rec(pParent->chan->index, ACTION_MIDI, frame_b, event_b);
}





void gPianoItem::remove() {
	recorder::deleteAction(pParent->chan->index, frame_a, ACTION_MIDI, true, event_a, 0.0);
	recorder::deleteAction(pParent->chan->index, frame_b, ACTION_MIDI, true, event_b, 0.0);

	

	((MidiChannel*) pParent->chan)->sendMidi(event_b);
}





int gPianoItem::handle(int e) {

	int ret = 0;

	switch (e) {

		case FL_ENTER: {
			selected = true;
			ret = 1;
			redraw();
			break;
		}

		case FL_LEAVE: {
			fl_cursor(FL_CURSOR_DEFAULT, FL_WHITE, FL_BLACK);
			selected = false;
			ret = 1;
			redraw();
			break;
		}

		case FL_MOVE: {
			onLeftEdge  = false;
			onRightEdge = false;

			if (Fl::event_x() >= x() && Fl::event_x() < x()+4) {
				onLeftEdge = true;
				fl_cursor(FL_CURSOR_WE, FL_WHITE, FL_BLACK);
			}
			else
			if (Fl::event_x() >= x()+w()-4 && Fl::event_x() <= x()+w()) {
				onRightEdge = true;
				fl_cursor(FL_CURSOR_WE, FL_WHITE, FL_BLACK);
			}
			else
				fl_cursor(FL_CURSOR_DEFAULT, FL_WHITE, FL_BLACK);

			ret = 1;
			break;
		}

		case FL_PUSH: {

			push_x = Fl::event_x() - x();
			old_x  = x();
			old_w  = w();

			if (Fl::event_button3()) {
				fl_cursor(FL_CURSOR_DEFAULT, FL_WHITE, FL_BLACK);
				remove();
				hide();   
				Fl::delete_widget(this);
				((gPianoRoll*)parent())->redraw();
			}
			ret = 1;
			break;
		}

		case FL_DRAG: {

			changed = true;

			gPianoRoll *pr = (gPianoRoll*) parent();
			int coverX     = pParent->coverX + pr->x(); 
			int nx, ny, nw;

			if (onLeftEdge) {
				nx = Fl::event_x();
				ny = y();
				nw = x()-Fl::event_x()+w();
				if (nx < pr->x()) {
					nx = pr->x();
					nw = w()+x()-pr->x();
				}
				else
				if (nx > x()+w()-8) {
					nx = x()+w()-8;
					nw = 8;
				}
				resize(nx, ny, nw, h());
			}
			else
			if (onRightEdge) {
				nw = Fl::event_x()-x();
				if (Fl::event_x() < x()+8)
					nw = 8;
				else
				if (Fl::event_x() > coverX)
					nw = coverX-x();
				size(nw, h());
			}
			else {
				nx = Fl::event_x() - push_x;
				if (nx < pr->x()+1)
					nx = pr->x()+1;
				else
				if (nx+w() > coverX)
					nx = coverX-w();

				

				if (pParent->gridTool->isOn())
					nx = pParent->gridTool->getSnapPoint(nx-pr->x()) + pr->x() - 1;

				position(nx, y());
			}

			

			redraw();
			((gPianoRoll*)parent())->redraw();
			ret = 1;
			break;
		}

		case FL_RELEASE: {

			

			if (overlap()) {
				resize(old_x, y(), old_w, h());
				redraw();
			}
			else
			if (changed) {
				remove();
				note    = getNote(getRelY());
				frame_a = getRelX() * pParent->zoom;
				frame_b = (getRelX()+w()) * pParent->zoom;
				record();
				changed = false;
			}

			((gPianoRoll*)parent())->redraw();

			ret = 1;
			break;
		}
	}
	return ret;
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Button.H>
#include <samplerate.h>
#include "ge_waveform.h"
#include "gd_editor.h"
#include "wave.h"
#include "glue.h"
#include "mixer.h"
#include "waveFx.h"
#include "ge_mixed.h"
#include "gg_waveTools.h"
#include "channel.h"
#include "sampleChannel.h"


extern Mixer G_Mixer;


gWaveform::gWaveform(int x, int y, int w, int h, class SampleChannel *ch, const char *l)
: Fl_Widget(x, y, w, h, l),
	chan(ch),
	menuOpen(false),
	chanStart(0),
	chanStartLit(false),
	chanEnd(0),
	chanEndLit(false),
	ratio(0.0f),
	selectionA(0),
	selectionB(0),
	selectionA_abs(0),
	selectionB_abs(0)
{
	data.sup  = NULL;
	data.inf  = NULL;
	data.size = 0;

	stretchToWindow();
}





gWaveform::~gWaveform() {
	freeData();
}





void gWaveform::freeData() {
	if (data.sup != NULL) {
		free(data.sup);
		free(data.inf);
		data.sup  = NULL;
		data.inf  = NULL;
		data.size = 0;
	}
}





int gWaveform::alloc(int datasize) {

	ratio = chan->wave->size / (float) datasize;

	if (ratio < 2)
		return 0;

	freeData();

	data.size = datasize;
	data.sup  = (int*) malloc(data.size * sizeof(int));
	data.inf  = (int*) malloc(data.size * sizeof(int));

	int offset = h() / 2;
	int zero   = y() + offset; 


	for (int i=0; i<data.size; i++) {

		int pp;  
		int pn;  

		

		pp = i * ((chan->wave->size - 1) / (float) datasize);
		pn = (i+1) * ((chan->wave->size - 1) / (float) datasize);

		if (pp % 2 != 0) pp -= 1;
		if (pn % 2 != 0) pn -= 1;

		float peaksup = 0.0f;
		float peakinf = 0.0f;

		int k = pp;
		while (k < pn) {
			if (chan->wave->data[k] > peaksup)
				peaksup = chan->wave->data[k];    
			else
			if (chan->wave->data[k] <= peakinf)
				peakinf = chan->wave->data[k];    
			k += 2;
		}

		data.sup[i] = zero - (peaksup * chan->boost * offset);
		data.inf[i] = zero - (peakinf * chan->boost * offset);

		

		if (data.sup[i] < y())       data.sup[i] = y();
		if (data.inf[i] > y()+h()-1) data.inf[i] = y()+h()-1;
	}
	recalcPoints();
	return 1;
}





void gWaveform::recalcPoints() {

	selectionA = relativePoint(selectionA_abs);
	selectionB = relativePoint(selectionB_abs);

	
	chanStart  = relativePoint(chan->begin / 2);

	

	
	if (chan->end == chan->wave->size)
		chanEnd = data.size - 2; 
	else
		
		chanEnd = relativePoint(chan->end / 2);
}





void gWaveform::draw() {

	

	fl_rectf(x(), y(), w(), h(), COLOR_BG_0);

	

	if (selectionA != selectionB) {

		int a_x = selectionA + x() - BORDER; 
		int b_x = selectionB + x() - BORDER; 

		if (a_x < 0)
			a_x = 0;
		if (b_x >= w()-1)
			b_x = w()-1;

		if (selectionA < selectionB)
			fl_rectf(a_x+BORDER, y(), b_x-a_x, h(), COLOR_BD_0);
		else
			fl_rectf(b_x+BORDER, y(), a_x-b_x, h(), COLOR_BD_0);
	}

	

	int offset = h() / 2;
	int zero   = y() + offset; 

	int wx1 = abs(x() - ((gWaveTools*)parent())->x());
	int wx2 = wx1 + ((gWaveTools*)parent())->w();
	if (x()+w() < ((gWaveTools*)parent())->w())
		wx2 = x() + w() - BORDER;

	fl_color(0, 0, 0);
	for (int i=wx1; i<wx2; i++) {
			fl_color(0, 0, 0);
			fl_line(i+x(), zero, i+x(), data.sup[i]);
			fl_line(i+x(), zero, i+x(), data.inf[i]);
		}

	

	fl_rect(x(), y(), w(), h(), COLOR_BD_0);

	

	int lineX = x()+chanStart+1;

	if (chanStartLit) fl_color(COLOR_BD_1);
	else              fl_color(COLOR_BD_0);

	

	fl_line(lineX, y()+1, lineX, y()+h()-2);

	

	if (lineX+FLAG_WIDTH > w()+x()-2)
		fl_rectf(lineX, y()+h()-FLAG_HEIGHT-1, w()-lineX+x()-1, FLAG_HEIGHT);
	else  {
		fl_rectf(lineX, y()+h()-FLAG_HEIGHT-1, FLAG_WIDTH, FLAG_HEIGHT);
		fl_color(255, 255, 255);
		fl_draw("s", lineX+4, y()+h()-3);
	}

	

	lineX = x()+chanEnd;
	if (chanEndLit)	fl_color(COLOR_BD_1);
	else            fl_color(COLOR_BD_0);

	fl_line(lineX, y()+1, lineX, y()+h()-2);

	if (lineX-FLAG_WIDTH < x())
		fl_rectf(x()+1, y()+1, lineX-x(), FLAG_HEIGHT);
	else {
		fl_rectf(lineX-FLAG_WIDTH, y()+1, FLAG_WIDTH, FLAG_HEIGHT);
		fl_color(255, 255, 255);
		fl_draw("e", lineX-10, y()+10);
	}
}





int gWaveform::handle(int e) {

	int ret = 0;

	switch (e) {

		case FL_PUSH: {

			mouseX = Fl::event_x();
			pushed = true;

			if (!mouseOnEnd() && !mouseOnStart()) {

				

				if (Fl::event_button3()) {
					openEditMenu();
				}
				else
				if (mouseOnSelectionA() || mouseOnSelectionB()) {
					resized = true;
				}
				else {
					dragged = true;
					selectionA = Fl::event_x() - x();

					if (selectionA >= data.size) selectionA = data.size;

					selectionB = selectionA;
					selectionA_abs = absolutePoint(selectionA);
					selectionB_abs = selectionA_abs;
				}
			}

			ret = 1;
			break;
		}

		case FL_RELEASE: {

			

			if (selectionA != selectionB) {
				pushed  = false;
				dragged = false;
				ret = 1;
				break;
			}

			
			
			int realChanStart = chan->begin;
			int realChanEnd   = chan->end;

			if (chanStartLit)
				realChanStart = absolutePoint(chanStart)*2;
			else
			if (chanEndLit)
				realChanEnd = absolutePoint(chanEnd)*2;

			glue_setBeginEndChannel((gdEditor *) window(), chan, realChanStart, realChanEnd, false);

			pushed  = false;
			dragged = false;

			redraw();
			ret = 1;
			break;
		}

		case FL_ENTER: {  
			ret = 1;
			break;
		}

		case FL_LEAVE: {
			if (chanStartLit || chanEndLit) {
				chanStartLit = false;
				chanEndLit   = false;
				redraw();
			}
			ret = 1;
			break;
		}

		case FL_MOVE: {
			mouseX = Fl::event_x();
			mouseY = Fl::event_y();

			if (mouseOnStart()) {
				chanStartLit = true;
				redraw();
			}
			else
			if (chanStartLit) {
				chanStartLit = false;
				redraw();
			}


			if (mouseOnEnd()) {
				chanEndLit = true;
				redraw();
			}
			else
			if (chanEndLit) {
				chanEndLit = false;
				redraw();
			}

			if (mouseOnSelectionA()) {
				fl_cursor(FL_CURSOR_WE, FL_WHITE, FL_BLACK);
			}
			else
			if (mouseOnSelectionB()) {
				fl_cursor(FL_CURSOR_WE, FL_WHITE, FL_BLACK);
			}
			else {
				fl_cursor(FL_CURSOR_DEFAULT, FL_WHITE, FL_BLACK);
			}

			ret = 1;
			break;
		}

		case FL_DRAG: {

			if (chanStartLit && pushed)	{

				chanStart += Fl::event_x() - mouseX;

				if (chanStart < 0)
					chanStart = 0;

				if (chanStart >= chanEnd)
					chanStart = chanEnd-2;

				redraw();
			}
			else
			if (chanEndLit && pushed) {

				chanEnd += Fl::event_x() - mouseX;

				if (chanEnd >= data.size - 2)
					chanEnd = data.size - 2;

				if (chanEnd <= chanStart)
					chanEnd = chanStart + 2;

				redraw();
			}

			

			else
			if (dragged) {

				selectionB = Fl::event_x() - x();

				if (selectionB >= data.size)
					selectionB = data.size;

				if (selectionB <= 0)
					selectionB = 0;

				selectionB_abs = absolutePoint(selectionB);
				redraw();
			}
			else
			if (resized) {
				if (mouseOnSelectionA()) {
					selectionA     = Fl::event_x() - x();
					selectionA_abs = absolutePoint(selectionA);
				}
				else {
					selectionB     = Fl::event_x() - x();
					selectionB_abs = absolutePoint(selectionB);
				}
				redraw();
			}
			mouseX = Fl::event_x();
			ret = 1;
			break;
		}
	}
	return ret;
}





bool gWaveform::mouseOnStart() {
	return mouseX-10 >  chanStart + x() - BORDER              &&
				 mouseX-10 <= chanStart + x() - BORDER + FLAG_WIDTH &&
				 mouseY    >  h() + y() - FLAG_HEIGHT;
}





bool gWaveform::mouseOnEnd() {
	return mouseX-10 >= chanEnd + x() - BORDER - FLAG_WIDTH &&
				 mouseX-10 <= chanEnd + x() - BORDER              &&
				 mouseY    <= y() + FLAG_HEIGHT + 1;
}





bool gWaveform::mouseOnSelectionA() {
	if (selectionA == selectionB)
		return false;
	return mouseX >= selectionA-5+x() && mouseX <= selectionA+5+x();
}





bool gWaveform::mouseOnSelectionB() {
	if (selectionA == selectionB)
		return false;
	return mouseX >= selectionB-5+x() && mouseX <= selectionB+5+x();
}





int gWaveform::absolutePoint(int p) {
	if (p <= 0)
		return 0;

	if (p > data.size)
		return chan->wave->size / 2;

	return (p * ratio) / 2;
}





int gWaveform::relativePoint(int p) {
	return (ceilf(p / ratio)) * 2;
}





void gWaveform::openEditMenu() {

	if (selectionA == selectionB)
		return;

	menuOpen = true;

	Fl_Menu_Item menu[] = {
		{"Cut"},
		{"Trim"},
		{"Silence"},
		{"Fade in"},
		{"Fade out"},
		{"Smooth edges"},
		{"Set start/end here"},
		{0}
	};

	if (chan->status == STATUS_PLAY) {
		menu[0].deactivate();
		menu[1].deactivate();
	}

	Fl_Menu_Button *b = new Fl_Menu_Button(0, 0, 100, 50);
	b->box(G_BOX);
	b->textsize(11);
	b->textcolor(COLOR_TEXT_0);
	b->color(COLOR_BG_0);

	const Fl_Menu_Item *m = menu->popup(Fl::event_x(), Fl::event_y(), 0, 0, b);
	if (!m) {
		menuOpen = false;
		return;
	}

	

	straightSel();

	if (strcmp(m->label(), "Silence") == 0) {
		wfx_silence(chan->wave, absolutePoint(selectionA), absolutePoint(selectionB));

		selectionA = 0;
		selectionB = 0;

		stretchToWindow();
		redraw();
		menuOpen = false;
		return;
	}

	if (strcmp(m->label(), "Set start/end here") == 0) {

		glue_setBeginEndChannel(
				(gdEditor *) window(), 
				chan,
				absolutePoint(selectionA) * 2,  
				absolutePoint(selectionB) * 2,  
				false, 
				false  
				);

		selectionA     = 0;
		selectionB     = 0;
		selectionA_abs = 0;
		selectionB_abs = 0;

		recalcPoints();
		redraw();
		menuOpen = false;
		return;
	}

	if (strcmp(m->label(), "Cut") == 0) {
		wfx_cut(chan->wave, absolutePoint(selectionA), absolutePoint(selectionB));

		

		glue_setBeginEndChannel(
			(gdEditor *) window(),
			chan,
			0,
			chan->wave->size,
			false);

		selectionA     = 0;
		selectionB     = 0;
		selectionA_abs = 0;
		selectionB_abs = 0;

		setZoom(0);

		menuOpen = false;
		return;
	}

	if (strcmp(m->label(), "Trim") == 0) {
		wfx_trim(chan->wave, absolutePoint(selectionA), absolutePoint(selectionB));

		glue_setBeginEndChannel(
			(gdEditor *) window(),
			chan,
			0,
			chan->wave->size,
			false);

		selectionA     = 0;
		selectionB     = 0;
		selectionA_abs = 0;
		selectionB_abs = 0;

		stretchToWindow();
		menuOpen = false;
		redraw();
		return;
	}

	if (!strcmp(m->label(), "Fade in") || !strcmp(m->label(), "Fade out")) {

		int type = !strcmp(m->label(), "Fade in") ? 0 : 1;
		wfx_fade(chan->wave, absolutePoint(selectionA), absolutePoint(selectionB), type);

		selectionA = 0;
		selectionB = 0;

		stretchToWindow();
		redraw();
		menuOpen = false;
		return;
	}

	if (!strcmp(m->label(), "Smooth edges")) {

		wfx_smooth(chan->wave, absolutePoint(selectionA), absolutePoint(selectionB));

		selectionA = 0;
		selectionB = 0;

		stretchToWindow();
		redraw();
		menuOpen = false;
		return;
	}
}





void gWaveform::straightSel() {
	if (selectionA > selectionB) {
		unsigned tmp = selectionB;
		selectionB = selectionA;
		selectionA = tmp;
	}
}





void gWaveform::setZoom(int type) {

	int newSize;
	if (type == -1) newSize = data.size*2;  
	else            newSize = data.size/2;  

	if (alloc(newSize)) {
		size(data.size, h());

		

		int shift;
		if (x() > 0)
			shift = Fl::event_x() - x();
		else
		if (type == -1)
			shift = Fl::event_x() + abs(x());
		else
			shift = (Fl::event_x() + abs(x())) / -2;

		if (x() - shift > BORDER)
			shift = 0;

		position(x() - shift, y());


		

		int  parentW = ((gWaveTools*)parent())->w();
		int  thisW   = x() + w() - BORDER;           

		if (thisW < parentW)
			position(x() + parentW - thisW, y());
		if (smaller())
			stretchToWindow();

		redraw();
	}
}





void gWaveform::stretchToWindow() {
	int s = ((gWaveTools*)parent())->w();
	alloc(s);
	position(BORDER, y());
	size(s, h());
}





bool gWaveform::smaller() {
	return w() < ((gWaveTools*)parent())->w();
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "ge_window.h"


gWindow::gWindow(int x, int y, int w, int h, const char *title, int id)
	: Fl_Double_Window(x, y, w, h, title), id(id), parent(NULL) { }





gWindow::gWindow(int w, int h, const char *title, int id)
	: Fl_Double_Window(w, h, title), id(id), parent(NULL) { }





gWindow::~gWindow() {

	

	for (unsigned i=0; i<subWindows.size; i++)
		delete subWindows.at(i);
	subWindows.clear();
}






void gWindow::cb_closeChild(Fl_Widget *v, void *p) {
	gWindow *child = (gWindow*) v;
	if (child->getParent() != NULL)
		(child->getParent())->delSubWindow(child);
}





void gWindow::addSubWindow(gWindow *w) {

	
	for (unsigned i=0; i<subWindows.size; i++)
		if (w->getId() == subWindows.at(i)->getId()) {
			
			delete w;
			return;
		}
	

	w->setParent(this);
	w->callback(cb_closeChild); 
	subWindows.add(w);
	
}





void gWindow::delSubWindow(gWindow *w) {
	for (unsigned i=0; i<subWindows.size; i++)
		if (w->getId() == subWindows.at(i)->getId()) {
			delete subWindows.at(i);
			subWindows.del(i);
			
			return;
		}
	
}





void gWindow::delSubWindow(int id) {
	for (unsigned i=0; i<subWindows.size; i++)
		if (subWindows.at(i)->getId() == id) {
			delete subWindows.at(i);
			subWindows.del(i);
			
			return;
		}
	
}





int gWindow::getId() {
	return id;
}





void gWindow::setId(int id) {
	this->id = id;
}





void gWindow::debug() {
	printf("---- window stack (id=%d): ----\n", getId());
	for (unsigned i=0; i<subWindows.size; i++)
		printf("[gWindow] %p (id=%d)\n", (void*)subWindows.at(i), subWindows.at(i)->getId());
	puts("----");
}





gWindow *gWindow::getParent() {
	return parent;
}





void gWindow::setParent(gWindow *w) {
	parent = w;
}





bool gWindow::hasWindow(int id) {
	for (unsigned i=0; i<subWindows.size; i++)
		if (id == subWindows.at(i)->getId())
			return true;
	return false;
}





gWindow *gWindow::getChild(int id) {
	for (unsigned i=0; i<subWindows.size; i++)
		if (id == subWindows.at(i)->getId())
			return subWindows.at(i);
	return NULL;
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "gg_keyboard.h"
#include "gd_browser.h"
#include "const.h"
#include "mixer.h"
#include "wave.h"
#include "gd_editor.h"
#include "conf.h"
#include "patch.h"
#include "gd_mainWindow.h"
#include "graphics.h"
#include "glue.h"
#include "recorder.h"
#include "gd_warnings.h"
#include "pluginHost.h"
#include "channel.h"
#include "sampleChannel.h"
#include "midiChannel.h"
#include "gd_keyGrabber.h"
#include "gd_midiGrabber.h"
#include "gd_midiOutputSetup.h"


extern Mixer 		     G_Mixer;
extern Conf  		     G_Conf;
extern Patch 		     G_Patch;
extern gdMainWindow *mainWin;


gChannel::gChannel(int X, int Y, int W, int H)
 : Fl_Group(X, Y, W, H, NULL)
{
}





gSampleChannel::gSampleChannel(int X, int Y, int W, int H, class SampleChannel *ch)
	: gChannel(X, Y, W, H), ch(ch)
{
	begin();

	button       = new gButton (x(), y(), 20, 20);
	status       = new gStatus (button->x()+button->w()+4, y(), 20, 20, ch);

#if defined(WITH_VST)
	sampleButton = new gClick  (status->x()+status->w()+4, y(), 213, 20, "-- no sample --");
#else
	sampleButton = new gClick  (status->x()+status->w()+4, y(), 237, 20, "-- no sample --");
#endif
	modeBox      = new gModeBox(sampleButton->x()+sampleButton->w()+4, y(), 20, 20, ch);
	mute         = new gClick  (modeBox->x()+modeBox->w()+4, y(), 20, 20, "", muteOff_xpm, muteOn_xpm);
	solo         = new gClick  (mute->x()+mute->w()+4, y(), 20, 20, "", soloOff_xpm, soloOn_xpm);

#if defined(WITH_VST)
	fx           = new gButton (solo->x()+solo->w()+4, y(), 20, 20, "", fxOff_xpm, fxOn_xpm);
	vol          = new gDial   (fx->x()+fx->w()+4, y(), 20, 20);
#else
	vol          = new gDial   (solo->x()+solo->w()+4, y(), 20, 20);
#endif

	readActions  = NULL; 

	end();

	update();

	button->callback(cb_button, (void*)this);
	button->when(FL_WHEN_CHANGED);   

#ifdef WITH_VST
	fx->callback(cb_openFxWindow, (void*)this);
#endif

	mute->type(FL_TOGGLE_BUTTON);
	mute->callback(cb_mute, (void*)this);

	solo->type(FL_TOGGLE_BUTTON);
	solo->callback(cb_solo, (void*)this);

	sampleButton->callback(cb_openMenu, (void*)this);
	vol->callback(cb_changeVol, (void*)this);

	ch->guiChannel = this;
}





void gSampleChannel::cb_button      (Fl_Widget *v, void *p) { ((gSampleChannel*)p)->__cb_button(); }
void gSampleChannel::cb_mute        (Fl_Widget *v, void *p) { ((gSampleChannel*)p)->__cb_mute(); }
void gSampleChannel::cb_solo        (Fl_Widget *v, void *p) { ((gSampleChannel*)p)->__cb_solo(); }
void gSampleChannel::cb_openMenu    (Fl_Widget *v, void *p) { ((gSampleChannel*)p)->__cb_openMenu(); }
void gSampleChannel::cb_changeVol   (Fl_Widget *v, void *p) { ((gSampleChannel*)p)->__cb_changeVol(); }
void gSampleChannel::cb_readActions (Fl_Widget *v, void *p) { ((gSampleChannel*)p)->__cb_readActions(); }
#ifdef WITH_VST
void gSampleChannel::cb_openFxWindow(Fl_Widget *v, void *p) { ((gSampleChannel*)p)->__cb_openFxWindow(); }
#endif





void gSampleChannel::__cb_mute() {
	glue_setMute(ch);
}





void gSampleChannel::__cb_solo() {
	solo->value() ? glue_setSoloOn(ch) : glue_setSoloOff(ch);
}





void gSampleChannel::__cb_changeVol() {
	glue_setChanVol(ch, vol->value());
}





#ifdef WITH_VST
void gSampleChannel::__cb_openFxWindow() {
	gu_openSubWindow(mainWin, new gdPluginList(PluginHost::CHANNEL, ch), WID_FX_LIST);
}
#endif





void gSampleChannel::__cb_button() {
	if (button->value())    
		glue_keyPress(ch, Fl::event_ctrl(), Fl::event_shift());
	else                    
		glue_keyRelease(ch, Fl::event_ctrl(), Fl::event_shift());
}





void gSampleChannel::__cb_openMenu() {

	

	if (G_Mixer.chanInput == ch || recorder::active)
		return;

	

	Fl_Menu_Item rclick_menu[] = {
		{"Load new sample..."},                     
		{"Export sample to file..."},               
		{"Setup keyboard input..."},                
		{"Setup MIDI input..."},                    
		{"Edit sample..."},                         
		{"Edit actions..."},                        
		{"Clear actions", 0, 0, 0, FL_SUBMENU},     
			{"All"},                                  
			{"Mute"},                                 
			{"Volume"},                               
			{"Start/Stop"},                           
			{0},                                      
		{"Free channel"},                           
		{"Delete channel"},                         
		{0}
	};

	if (ch->status & (STATUS_EMPTY | STATUS_MISSING)) {
		rclick_menu[1].deactivate();
		rclick_menu[4].deactivate();
		rclick_menu[12].deactivate();
	}

	

	if (!ch->hasActions)
		rclick_menu[6].deactivate();

	

	if (ch->mode & LOOP_ANY)
		rclick_menu[10].deactivate();

	Fl_Menu_Button *b = new Fl_Menu_Button(0, 0, 100, 50);
	b->box(G_BOX);
	b->textsize(11);
	b->textcolor(COLOR_TEXT_0);
	b->color(COLOR_BG_0);

	const Fl_Menu_Item *m = rclick_menu->popup(Fl::event_x(), Fl::event_y(), 0, 0, b);
	if (!m) return;

	if (strcmp(m->label(), "Load new sample...") == 0) {
		openBrowser(BROWSER_LOAD_SAMPLE);
		return;
	}

	if (strcmp(m->label(), "Setup keyboard input...") == 0) {
		new gdKeyGrabber(ch); 
		return;
	}

	if (strcmp(m->label(), "Setup MIDI input...") == 0) {
		gu_openSubWindow(mainWin, new gdMidiGrabberChannel(ch), 0);
		return;
	}

	if (strcmp(m->label(), "Edit sample...") == 0) {
		gu_openSubWindow(mainWin, new gdEditor(ch), WID_SAMPLE_EDITOR); 
		return;
	}

	if (strcmp(m->label(), "Export sample to file...") == 0) {
		openBrowser(BROWSER_SAVE_SAMPLE);
		return;
	}

	if (strcmp(m->label(), "Delete channel") == 0) {
		if (!gdConfirmWin("Warning", "Delete channel: are you sure?"))
			return;
		glue_deleteChannel(ch);
		return;
	}

	if (strcmp(m->label(), "Free channel") == 0) {
		if (ch->status == STATUS_PLAY) {
			if (!gdConfirmWin("Warning", "This action will stop the channel: are you sure?"))
				return;
		}
		else if (!gdConfirmWin("Warning", "Free channel: are you sure?"))
			return;

		glue_freeChannel(ch);

		

		

		mainWin->delSubWindow(WID_FILE_BROWSER);
		mainWin->delSubWindow(WID_ACTION_EDITOR);
		mainWin->delSubWindow(WID_SAMPLE_EDITOR);
		mainWin->delSubWindow(WID_FX_LIST);

		return;
	}

	if (strcmp(m->label(), "Mute") == 0) {
		if (!gdConfirmWin("Warning", "Clear all mute actions: are you sure?"))
			return;
		recorder::clearAction(ch->index, ACTION_MUTEON | ACTION_MUTEOFF);
		if (!ch->hasActions)
			delActionButton();

		

		gu_refreshActionEditor(); 
		return;
	}

	if (strcmp(m->label(), "Start/Stop") == 0) {
		if (!gdConfirmWin("Warning", "Clear all start/stop actions: are you sure?"))
			return;
		recorder::clearAction(ch->index, ACTION_KEYPRESS | ACTION_KEYREL | ACTION_KILLCHAN);
		if (!ch->hasActions)
			delActionButton();
		gu_refreshActionEditor();  
		return;
	}

	if (strcmp(m->label(), "Volume") == 0) {
		if (!gdConfirmWin("Warning", "Clear all volume actions: are you sure?"))
			return;
		recorder::clearAction(ch->index, ACTION_VOLUME);
		if (!ch->hasActions)
			delActionButton();
		gu_refreshActionEditor();  
		return;
	}

	if (strcmp(m->label(), "All") == 0) {
		if (!gdConfirmWin("Warning", "Clear all actions: are you sure?"))
			return;
		recorder::clearChan(ch->index);
		delActionButton();
		gu_refreshActionEditor(); 
		return;
	}

	if (strcmp(m->label(), "Edit actions...") == 0) {
		gu_openSubWindow(mainWin, new gdActionEditor(ch),	WID_ACTION_EDITOR);
		return;
	}
}





void gSampleChannel::__cb_readActions() {
	ch->readActions ? glue_stopReadingRecs(ch) : glue_startReadingRecs(ch);
}





void gSampleChannel::openBrowser(int type) {
	const char *title = "";
	switch (type) {
		case BROWSER_LOAD_SAMPLE:
			title = "Browse Sample";
			break;
		case BROWSER_SAVE_SAMPLE:
			title = "Save Sample";
			break;
		case -1:
			title = "Edit Sample";
			break;
	}
	gWindow *childWin = new gdBrowser(title, G_Conf.samplePath, ch, type);
	gu_openSubWindow(mainWin, childWin,	WID_FILE_BROWSER);
}





void gSampleChannel::refresh() {

	if (ch->status == STATUS_OFF) {
		sampleButton->bgColor0 = COLOR_BG_0;
		sampleButton->bdColor  = COLOR_BD_0;
		sampleButton->txtColor = COLOR_TEXT_0;
	}
	else
	if (ch->status == STATUS_PLAY) {
		sampleButton->bgColor0 = COLOR_BG_2;
		sampleButton->bdColor  = COLOR_BD_1;
		sampleButton->txtColor = COLOR_TEXT_1;
	}
	else
	if (ch->status & (STATUS_WAIT | STATUS_ENDING))
		__gu_blinkChannel(this);    

	if (ch->recStatus & (REC_WAITING | REC_ENDING))
		__gu_blinkChannel(this);    

	if (ch->wave != NULL) {

		if (G_Mixer.chanInput == ch)
			sampleButton->bgColor0 = COLOR_BG_3;

		if (recorder::active) {
			if (recorder::canRec(ch)) {
				sampleButton->bgColor0 = COLOR_BG_4;
				sampleButton->txtColor = COLOR_TEXT_0;
			}
		}
		status->redraw();
	}
	sampleButton->redraw();
}





void gSampleChannel::reset() {
	sampleButton->bgColor0 = COLOR_BG_0;
	sampleButton->bdColor  = COLOR_BD_0;
	sampleButton->txtColor = COLOR_TEXT_0;
	sampleButton->label("-- no sample --");
	delActionButton(true); 
 	sampleButton->redraw();
	status->redraw();
}





void gSampleChannel::update() {

	

	switch (ch->status) {
		case STATUS_EMPTY:
			sampleButton->label("-- no sample --");
			break;
		case STATUS_MISSING:
		case STATUS_WRONG:
			sampleButton->label("* file not found! *");
			break;
		default:
			gu_trim_label(ch->wave->name.c_str(), 28, sampleButton);
			break;
	}

	

	if (ch->hasActions)
		addActionButton();
	else
		delActionButton();

	

	char k[4];
	sprintf(k, "%c", ch->key);
	button->copy_label(k);
	button->redraw();

	

	modeBox->value(ch->mode);
	modeBox->redraw();

	

	vol->value(ch->volume);
	mute->value(ch->mute);
	solo->value(ch->solo);
}





int gSampleChannel::keyPress(int e) {
	int ret;
	if (e == FL_KEYDOWN && button->value())                              
		ret = 1;
	else
	if (Fl::event_key() == ch->key && !button->value()) {
		button->take_focus();                                              
		button->value((e == FL_KEYDOWN || e == FL_SHORTCUT) ? 1 : 0);      
		button->do_callback();                                             
		ret = 1;
	}
	else
		ret = 0;

	if (Fl::event_key() == ch->key)
		button->value((e == FL_KEYDOWN || e == FL_SHORTCUT) ? 1 : 0);      

	return ret;
}





void gSampleChannel::addActionButton() {

	

	if (readActions != NULL)
		return;

	sampleButton->size(sampleButton->w()-24, sampleButton->h());

	redraw();

	readActions = new gClick(sampleButton->x() + sampleButton->w() + 4, sampleButton->y(), 20, 20, "", readActionOff_xpm, readActionOn_xpm);
	readActions->type(FL_TOGGLE_BUTTON);
	readActions->value(ch->readActions);
	readActions->callback(cb_readActions, (void*)this);
	add(readActions);

	

	mainWin->keyboard->redraw();
}





void gSampleChannel::delActionButton(bool force) {
	if (!force && (readActions == NULL || ch->hasActions))
		return;

	remove(readActions);		
	delete readActions;     
	readActions = NULL;

	sampleButton->size(sampleButton->w()+24, sampleButton->h());
	sampleButton->redraw();
}







gMidiChannel::gMidiChannel(int X, int Y, int W, int H, class MidiChannel *ch)
	: gChannel(X, Y, W, H), ch(ch)
{
	begin();

	button       = new gButton (x(), y(), 20, 20);

#if defined(WITH_VST)
	sampleButton = new gClick (button->x()+button->w()+4, y(), 261, 20, "-- MIDI --");
#else
	sampleButton = new gClick (button->x()+button->w()+4, y(), 285, 20, "-- MIDI --");
#endif

	mute         = new gClick (sampleButton->x()+sampleButton->w()+4, y(), 20, 20, "", muteOff_xpm, muteOn_xpm);
	solo         = new gClick (mute->x()+mute->w()+4, y(), 20, 20, "", soloOff_xpm, soloOn_xpm);

#if defined(WITH_VST)
	fx           = new gButton(solo->x()+solo->w()+4, y(), 20, 20, "", fxOff_xpm, fxOn_xpm);
	vol          = new gDial  (fx->x()+fx->w()+4, y(), 20, 20);
#else
	vol          = new gDial  (solo->x()+solo->w()+4, y(), 20, 20);
#endif

	end();

	update();

	button->callback(cb_button, (void*)this);
	button->when(FL_WHEN_CHANGED);   

#ifdef WITH_VST
	fx->callback(cb_openFxWindow, (void*)this);
#endif

	mute->type(FL_TOGGLE_BUTTON);
	mute->callback(cb_mute, (void*)this);

	solo->type(FL_TOGGLE_BUTTON);
	solo->callback(cb_solo, (void*)this);

	sampleButton->callback(cb_openMenu, (void*)this);
	vol->callback(cb_changeVol, (void*)this);

	ch->guiChannel = this;
}





void gMidiChannel::cb_button      (Fl_Widget *v, void *p) { ((gMidiChannel*)p)->__cb_button(); }
void gMidiChannel::cb_mute        (Fl_Widget *v, void *p) { ((gMidiChannel*)p)->__cb_mute(); }
void gMidiChannel::cb_solo        (Fl_Widget *v, void *p) { ((gMidiChannel*)p)->__cb_solo(); }
void gMidiChannel::cb_openMenu    (Fl_Widget *v, void *p) { ((gMidiChannel*)p)->__cb_openMenu(); }
void gMidiChannel::cb_changeVol   (Fl_Widget *v, void *p) { ((gMidiChannel*)p)->__cb_changeVol(); }
#ifdef WITH_VST
void gMidiChannel::cb_openFxWindow(Fl_Widget *v, void *p) { ((gMidiChannel*)p)->__cb_openFxWindow(); }
#endif





void gMidiChannel::__cb_mute() {
	glue_setMute(ch);
}





void gMidiChannel::__cb_solo() {
	solo->value() ? glue_setSoloOn(ch) : glue_setSoloOff(ch);
}





void gMidiChannel::__cb_changeVol() {
	glue_setChanVol(ch, vol->value());
}





#ifdef WITH_VST
void gMidiChannel::__cb_openFxWindow() {
	gu_openSubWindow(mainWin, new gdPluginList(PluginHost::CHANNEL, ch), WID_FX_LIST);
}
#endif




void gMidiChannel::__cb_button() {
	if (button->value())
		glue_keyPress(ch, Fl::event_ctrl(), Fl::event_shift());
}





void gMidiChannel::__cb_openMenu() {

	Fl_Menu_Item rclick_menu[] = {
		{"Edit actions..."},                        
		{"Clear actions", 0, 0, 0, FL_SUBMENU},     
			{"All"},                                  
			{0},                                      
		{"Setup MIDI output..."},                   
		{"Setup MIDI input..."},                    
		{"Delete channel"},                         
		{0}
	};

	

	if (!ch->hasActions)
		rclick_menu[1].deactivate();

	Fl_Menu_Button *b = new Fl_Menu_Button(0, 0, 100, 50);
	b->box(G_BOX);
	b->textsize(11);
	b->textcolor(COLOR_TEXT_0);
	b->color(COLOR_BG_0);

	const Fl_Menu_Item *m = rclick_menu->popup(Fl::event_x(), Fl::event_y(), 0, 0, b);
	if (!m) return;

	if (strcmp(m->label(), "Delete channel") == 0) {
		if (!gdConfirmWin("Warning", "Delete channel: are you sure?"))
			return;
		glue_deleteChannel(ch);
		return;
	}

	if (strcmp(m->label(), "All") == 0) {
		if (!gdConfirmWin("Warning", "Clear all actions: are you sure?"))
			return;
		recorder::clearChan(ch->index);
		gu_refreshActionEditor(); 
		return;
	}

	if (strcmp(m->label(), "Edit actions...") == 0) {
		gu_openSubWindow(mainWin, new gdActionEditor(ch),	WID_ACTION_EDITOR);
		return;
	}

	if (strcmp(m->label(), "Setup MIDI output...") == 0) {
		gu_openSubWindow(mainWin, new gdMidiOutputSetup(ch), 0);
		return;
	}

	if (strcmp(m->label(), "Setup MIDI input...") == 0) {
		gu_openSubWindow(mainWin, new gdMidiGrabberChannel(ch), 0);
		return;
	}
}





void gMidiChannel::refresh() {

	if (ch->status == STATUS_OFF) {
		sampleButton->bgColor0 = COLOR_BG_0;
		sampleButton->bdColor  = COLOR_BD_0;
		sampleButton->txtColor = COLOR_TEXT_0;
	}
	else
	if (ch->status == STATUS_PLAY) {
		sampleButton->bgColor0 = COLOR_BG_2;
		sampleButton->bdColor  = COLOR_BD_1;
		sampleButton->txtColor = COLOR_TEXT_1;
	}
	else
	if (ch->status & (STATUS_WAIT | STATUS_ENDING))
		__gu_blinkChannel(this);    

	if (ch->recStatus & (REC_WAITING | REC_ENDING))
		__gu_blinkChannel(this);    

	sampleButton->redraw();
}





void gMidiChannel::reset() {
	sampleButton->bgColor0 = COLOR_BG_0;
	sampleButton->bdColor  = COLOR_BD_0;
	sampleButton->txtColor = COLOR_TEXT_0;
	sampleButton->label("-- MIDI --");
	sampleButton->redraw();
}





void gMidiChannel::update() {

	if (ch->midiOut) {
		char tmp[32];
		sprintf(tmp, "-- MIDI (channel %d) --", ch->midiOutChan+1);
		sampleButton->copy_label(tmp);
	}
	else
		sampleButton->label("-- MIDI --");

	vol->value(ch->volume);
	mute->value(ch->mute);
	solo->value(ch->solo);
}





int gMidiChannel::keyPress(int e) {
	return 1; 
}







Keyboard::Keyboard(int X, int Y, int W, int H, const char *L)
: Fl_Scroll(X, Y, W, H, L),
	bckspcPressed(false),
	endPressed(false),
	spacePressed(false)
{
	color(COLOR_BG_MAIN);
	type(Fl_Scroll::VERTICAL);
	scrollbar.color(COLOR_BG_0);
	scrollbar.selection_color(COLOR_BG_1);
	scrollbar.labelcolor(COLOR_BD_1);
	scrollbar.slider(G_BOX);

	gChannelsL  = new Fl_Group(x(), y(), (w()/2)-16, 0);
	gChannelsR  = new Fl_Group(gChannelsL->x()+gChannelsL->w()+32, y(), (w()/2)-16, 0);
	addChannelL = new gClick(gChannelsL->x(), gChannelsL->y()+gChannelsL->h(), gChannelsL->w(), 20, "Add new channel");
	addChannelR = new gClick(gChannelsR->x(), gChannelsR->y()+gChannelsR->h(), gChannelsR->w(), 20, "Add new channel");

	

	add(addChannelL);
	add(addChannelR);
	add(gChannelsL);
	add(gChannelsR);

	gChannelsL->resizable(NULL);
	gChannelsR->resizable(NULL);

	addChannelL->callback(cb_addChannelL, (void*) this);
	addChannelR->callback(cb_addChannelR, (void*) this);
}





int Keyboard::openChanTypeMenu() {

	Fl_Menu_Item rclick_menu[] = {
		{"Sample channel"},
		{"MIDI channel"},
		{0}
	};

	Fl_Menu_Button *b = new Fl_Menu_Button(0, 0, 100, 50);
	b->box(G_BOX);
	b->textsize(11);
	b->textcolor(COLOR_TEXT_0);
	b->color(COLOR_BG_0);

	const Fl_Menu_Item *m = rclick_menu->popup(Fl::event_x(), Fl::event_y(), 0, 0, b);
	if (!m) return 0;

	if (strcmp(m->label(), "Sample channel") == 0)
		return CHANNEL_SAMPLE;
	if (strcmp(m->label(), "MIDI channel") == 0)
		return CHANNEL_MIDI;
	return 0;
}





void Keyboard::fixRightColumn() {
	if (!hasScrollbar())
		gChannelsR->position(gChannelsL->x()+gChannelsL->w()+32, gChannelsR->y());
	else
		gChannelsR->position(gChannelsL->x()+gChannelsL->w()+8, gChannelsR->y());
	addChannelR->position(gChannelsR->x(), addChannelR->y());
}





void Keyboard::freeChannel(gChannel *gch) {
	gch->reset();
}





void Keyboard::deleteChannel(gChannel *gch) {
	Fl::lock();
	gch->hide();
	gChannelsR->remove(gch);
	gChannelsL->remove(gch);
	delete gch;
	
	Fl::unlock();
	fixRightColumn();
}





void Keyboard::updateChannel(gChannel *gch) {
	gch->update();
}





void Keyboard::updateChannels(char side) {

	Fl_Group *group;
	gClick   *add;

	if (side == 0)	{
		group = gChannelsL;
		add   = addChannelL;
	}
	else {
		group = gChannelsR;
		add   = addChannelR;
	}

	

	for (int i=0; i<group->children(); i++) {
		gChannel *gch = (gChannel*) group->child(i);
		gch->position(gch->x(), group->y()+(i*24));
	}
	group->size(group->w(), group->children()*24);
	add->position(add->x(), group->y()+group->h());

	redraw();
}





void Keyboard::cb_addChannelL(Fl_Widget *v, void *p) { ((Keyboard*)p)->__cb_addChannelL(); }
void Keyboard::cb_addChannelR(Fl_Widget *v, void *p) { ((Keyboard*)p)->__cb_addChannelR(); }





gChannel *Keyboard::addChannel(char side, Channel *ch) {
	Fl_Group *group;
	gClick   *add;

	if (side == 0) {
		group = gChannelsL;
		add   = addChannelL;
	}
	else {
		group = gChannelsR;
		add   = addChannelR;
	}

	gChannel *gch = NULL;

	if (ch->type == CHANNEL_SAMPLE)
		gch = (gSampleChannel*) new gSampleChannel(
				group->x(),
				group->y() + group->children() * 24,
				group->w(),
				20,
				(SampleChannel*) ch);
	else
		gch = (gMidiChannel*) new gMidiChannel(
				group->x(),
				group->y() + group->children() * 24,
				group->w(),
				20,
				(MidiChannel*) ch);

	group->add(gch);
	group->size(group->w(), group->children() * 24);
	add->position(group->x(), group->y()+group->h());
	fixRightColumn();
	redraw();

	return gch;
}





bool Keyboard::hasScrollbar() {
	if (24 * (gChannelsL->children()) > h())
		return true;
	if (24 * (gChannelsR->children()) > h())
		return true;
	return false;
}





void Keyboard::__cb_addChannelL() {
	int type = openChanTypeMenu();
	if (type)
		glue_addChannel(0, type);
}


void Keyboard::__cb_addChannelR() {
	int type = openChanTypeMenu();
	if (type)
		glue_addChannel(1, type);
}





int Keyboard::handle(int e) {
	int ret = Fl_Group::handle(e);  
	switch (e) {
		case FL_FOCUS:
		case FL_UNFOCUS: {
			ret = 1;                	
			break;
		}
		case FL_SHORTCUT:           
		case FL_KEYDOWN:            
		case FL_KEYUP: {            

			

			if (e == FL_KEYDOWN) {
				if (Fl::event_key() == FL_BackSpace && !bckspcPressed) {
					bckspcPressed = true;
					glue_rewindSeq();
					ret = 1;
					break;
				}
				else if (Fl::event_key() == FL_End && !endPressed) {
					endPressed = true;
					glue_startStopInputRec(false);  
					ret = 1;
					break;
				}
				else if (Fl::event_key() == FL_Enter && !enterPressed) {
					enterPressed = true;
					glue_startStopActionRec();
					ret = 1;
					break;
				}
				else if (Fl::event_key() == ' ' && !spacePressed) {
					spacePressed = true;
					G_Mixer.running ? glue_stopSeq() : glue_startSeq();
					ret = 1;
					break;
				}
			}
			else if (e == FL_KEYUP) {
				if (Fl::event_key() == FL_BackSpace)
					bckspcPressed = false;
				else if (Fl::event_key() == FL_End)
					endPressed = false;
				else if (Fl::event_key() == ' ')
					spacePressed = false;
				else if (Fl::event_key() == FL_Enter)
					enterPressed = false;
			}

			

			for (int i=0; i<gChannelsL->children(); i++)
				ret &= ((gChannel*)gChannelsL->child(i))->keyPress(e);
			for (int i=0; i<gChannelsR->children(); i++)
				ret &= ((gChannel*)gChannelsR->child(i))->keyPress(e);
			break;
		}
	}
	return ret;
}





void Keyboard::clear() {
	Fl::lock();
	gChannelsL->clear();
	gChannelsR->clear();
	for (unsigned i=0; i<G_Mixer.channels.size; i++)
		G_Mixer.channels.at(i)->guiChannel = NULL;
	Fl::unlock();

	gChannelsR->size(gChannelsR->w(), 0);
	gChannelsL->size(gChannelsL->w(), 0);

	gChannelsL->resizable(NULL);
	gChannelsR->resizable(NULL);

	addChannelL->position(gChannelsL->x(), gChannelsL->y()+gChannelsL->h());
	addChannelR->position(gChannelsR->x(), gChannelsR->y()+gChannelsR->h());

	redraw();
}





void Keyboard::setChannelWithActions(gSampleChannel *gch) {
	if (gch->ch->hasActions)
		gch->addActionButton();
	else
		gch->delActionButton();
}

/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "gg_waveTools.h"
#include "graphics.h"
#include "ge_mixed.h"
#include "ge_waveform.h"
#include "mixer.h"


gWaveTools::gWaveTools(int x, int y, int w, int h, SampleChannel *ch, const char *l)
	: Fl_Scroll(x, y, w, h, l)
{
	type(Fl_Scroll::HORIZONTAL_ALWAYS);
	hscrollbar.color(COLOR_BG_0);
	hscrollbar.selection_color(COLOR_BG_1);
	hscrollbar.labelcolor(COLOR_BD_1);
	hscrollbar.slider(G_BOX);

	waveform = new gWaveform(x, y, w, h-24, ch);


	
}






void gWaveTools::updateWaveform() {
	waveform->alloc(w());
	waveform->redraw();
}





void gWaveTools::resize(int x, int y, int w, int h) {
	if (this->w() == w || (this->w() != w && this->h() != h)) {   
		Fl_Widget::resize(x, y, w, h);
		waveform->resize(x, y, waveform->w(), h-24);
		updateWaveform();
	}
	else {                                                        
		Fl_Widget::resize(x, y, w, h);
	}

	if (this->w() > waveform->w())
		waveform->stretchToWindow();

	int offset = waveform->x() + waveform->w() - this->w() - this->x();
	if (offset < 0)
		waveform->position(waveform->x()-offset, this->y());
}





int gWaveTools::handle(int e) {
	int ret = Fl_Group::handle(e);
	switch (e) {
		case FL_MOUSEWHEEL: {
			waveform->setZoom(Fl::event_dy());
			redraw();
			ret = 1;
			break;
		}
	}
	return ret;
}

/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "glue.h"
#include "ge_waveform.h"
#include "mixerHandler.h"
#include "gui_utils.h"
#include "ge_mixed.h"
#include "mixer.h"
#include "recorder.h"
#include "gd_mainWindow.h"
#include "gd_editor.h"
#include "wave.h"
#include "gd_warnings.h"
#include "pluginHost.h"
#include "gg_waveTools.h"
#include "channel.h"
#include "sampleChannel.h"
#include "midiChannel.h"
#include "utils.h"
#include "kernelMidi.h"


extern gdMainWindow *mainWin;
extern Mixer	   		 G_Mixer;
extern Patch     		 G_Patch;
extern Conf	 	   		 G_Conf;
extern bool 		 		 G_audio_status;
#ifdef WITH_VST
extern PluginHost		 G_PluginHost;
#endif


static bool __soloSession__ = false;





int glue_loadChannel(SampleChannel *ch, const char *fname, const char *fpath) {

	

	G_Conf.setPath(G_Conf.samplePath, fpath);

	int result = ch->load(fname);

	if (result == SAMPLE_LOADED_OK)
		mainWin->keyboard->updateChannel(ch->guiChannel);

	if (G_Conf.fullChanVolOnLoad)
		glue_setChanVol(ch, 1.0, false); 

	return result;
}





Channel *glue_addChannel(int side, int type) {
	Channel *ch    = G_Mixer.addChannel(side, type);
	gChannel *gch  = mainWin->keyboard->addChannel(side, ch);
	ch->guiChannel = gch;
	return ch;
}





int glue_loadPatch(const char *fname, const char *fpath, gProgress *status, bool isProject) {

	

	status->show();
	status->value(0.1f);
	
	Fl::wait(0);

	

	int res = G_Patch.open(fname);
	if (res != PATCH_OPEN_OK)
		return res;

	

	if (res)
		gu_closeAllSubwindows();

	

	glue_resetToInitState(false);

	status->value(0.2f);  
	
	Fl::wait(0);

	

	mh_loadPatch(isProject, fname);

	

	G_Patch.getName();
	gu_update_win_label(G_Patch.name);

	status->value(0.4f);  
	
	Fl::wait(0);

	G_Patch.readRecs();
	status->value(0.6f);  
	
	Fl::wait(0);

#ifdef WITH_VST
	int resPlugins = G_Patch.readPlugins();
	status->value(0.8f);  
	
	Fl::wait(0);
#endif

	

	recorder::updateSamplerate(G_Conf.samplerate, G_Patch.samplerate);

	

	gu_update_controls();

	status->value(1.0f);  
	
	Fl::wait(0);

	

	G_Conf.setPath(G_Conf.patchPath, fpath);

	printf("[glue] patch %s loaded\n", fname);

#ifdef WITH_VST
	if (resPlugins != 1)
		gdAlert("Some VST files were not loaded successfully.");
#endif

	return res;
}





int glue_savePatch(const char *fullpath, const char *name, bool isProject) {

	if (G_Patch.write(fullpath, name, isProject) == 1) {
		strcpy(G_Patch.name, name);
		G_Patch.name[strlen(name)] = '\0';
		gu_update_win_label(name);
		printf("[glue] patch saved as %s\n", fullpath);
		return 1;
	}
	else
		return 0;
}





void glue_deleteChannel(Channel *ch) {
	int index = ch->index;
	int side  = ch->side;
	recorder::clearChan(index);
	mainWin->keyboard->deleteChannel(ch->guiChannel);
	G_Mixer.deleteChannel(ch);
	mainWin->keyboard->updateChannels(side);
}





void glue_freeChannel(Channel *ch) {
	mainWin->keyboard->freeChannel(ch->guiChannel);
	recorder::clearChan(ch->index);
	ch->empty();
}





void glue_setBpm(const char *v1, const char *v2) {
	char  buf[6];
	float value = atof(v1) + (atof(v2)/10);
	if (value < 20.0f)	{
		value = 20.0f;
		sprintf(buf, "20.0");
	}
	else
		sprintf(buf, "%s.%s", v1, !strcmp(v2, "") ? "0" : v2);

	

	float old_bpm = G_Mixer.bpm;
	G_Mixer.bpm = value;
	G_Mixer.updateFrameBars();

	

	recorder::updateBpm(old_bpm, value, G_Mixer.quanto);
	gu_refreshActionEditor();

	mainWin->bpm->copy_label(buf);
	printf("[glue] Bpm changed to %s (real=%f)\n", buf, G_Mixer.bpm);
}





void glue_setBeats(int beats, int bars, bool expand) {

	

	int      oldvalue = G_Mixer.beats;
	unsigned oldfpb		= G_Mixer.totalFrames;

	if (beats > MAX_BEATS)
		G_Mixer.beats = MAX_BEATS;
	else if (beats < 1)
		G_Mixer.beats = 1;
	else
		G_Mixer.beats = beats;

	

	if (bars > G_Mixer.beats)
		G_Mixer.bars = G_Mixer.beats;
	else if (bars <= 0)
		G_Mixer.bars = 1;
	else if (beats % bars != 0) {
		G_Mixer.bars = bars + (beats % bars);
		if (beats % G_Mixer.bars != 0) 
			G_Mixer.bars = G_Mixer.bars - (beats % G_Mixer.bars);
	}
	else
		G_Mixer.bars = bars;

	G_Mixer.updateFrameBars();

	

	if (expand) {
		if (G_Mixer.beats > oldvalue)
			recorder::expand(oldfpb, G_Mixer.totalFrames);
		
		
	}

	char buf_batt[8];
	sprintf(buf_batt, "%d/%d", G_Mixer.beats, G_Mixer.bars);
	mainWin->beats->copy_label(buf_batt);

	

	gu_refreshActionEditor();
}





void glue_startStopSeq(bool gui) {
	G_Mixer.running ? glue_stopSeq(gui) : glue_startSeq(gui);
}





void glue_startSeq(bool gui) {

	G_Mixer.running = true;

	if (gui) {
#ifdef __linux__
		kernelAudio::jackStart();
#endif
	}

	if (G_Conf.midiSync == MIDI_SYNC_CLOCK_M) {
		kernelMidi::send(MIDI_START, -1, -1);
		kernelMidi::send(MIDI_POSITION_PTR, 0, 0);
	}

	Fl::lock();
	mainWin->beat_stop->value(1);
	mainWin->beat_stop->redraw();
	Fl::unlock();
}





void glue_stopSeq(bool gui) {

	mh_stopSequencer();

	if (G_Conf.midiSync == MIDI_SYNC_CLOCK_M)
		kernelMidi::send(MIDI_STOP, -1, -1);

#ifdef __linux__
	if (gui)
		kernelAudio::jackStop();
#endif

	

	if (recorder::active) {
		recorder::active = false;
		Fl::lock();
		mainWin->beat_rec->value(0);
		mainWin->beat_rec->redraw();
		Fl::unlock();
	}

	

	if (G_Mixer.chanInput != NULL) {
		mh_stopInputRec();
		Fl::lock();
		mainWin->input_rec->value(0);
		mainWin->input_rec->redraw();
		Fl::unlock();
	}

	Fl::lock();
	mainWin->beat_stop->value(0);
	mainWin->beat_stop->redraw();
	Fl::unlock();
}





void glue_rewindSeq() {
	mh_rewindSequencer();
	if (G_Conf.midiSync == MIDI_SYNC_CLOCK_M)
		kernelMidi::send(MIDI_POSITION_PTR, 0, 0);
}





void glue_startStopActionRec() {
	recorder::active ? glue_stopActionRec() : glue_startActionRec();
}





void glue_startActionRec() {
	if (G_audio_status == false)
		return;
	if (!G_Mixer.running)
		glue_startSeq();	        
	recorder::active = true;

	Fl::lock();
	mainWin->beat_rec->value(1);
	mainWin->beat_rec->redraw();
	Fl::unlock();
}





void glue_stopActionRec() {

	

	recorder::active = false;
	recorder::sortActions();

	for (unsigned i=0; i<G_Mixer.channels.size; i++)
		if (G_Mixer.channels.at(i)->type == CHANNEL_SAMPLE) {
			SampleChannel *ch = (SampleChannel*) G_Mixer.channels.at(i);
			if (ch->hasActions)
				ch->readActions = true;
			else
				ch->readActions = false;
			mainWin->keyboard->setChannelWithActions((gSampleChannel*)ch->guiChannel);
		}

	Fl::lock();
	mainWin->beat_rec->value(0);
	mainWin->beat_rec->redraw();
	Fl::unlock();

	

	gu_refreshActionEditor();
}





void glue_startStopReadingRecs(SampleChannel *ch, bool gui) {
	if (ch->readActions)
		glue_stopReadingRecs(ch, gui);
	else
		glue_startReadingRecs(ch, gui);
}





void glue_startReadingRecs(SampleChannel *ch, bool gui) {
	if (G_Conf.treatRecsAsLoops)
		ch->recStatus = REC_WAITING;
	else
		ch->setReadActions(true);
	if (!gui) {
		gSampleChannel *gch = (gSampleChannel*)ch->guiChannel;
		if (gch->readActions) { 
			Fl::lock();
			gch->readActions->value(1);
			Fl::unlock();
		}
	}
}





void glue_stopReadingRecs(SampleChannel *ch, bool gui) {

	

	if (G_Conf.treatRecsAsLoops)
		ch->recStatus = REC_ENDING;
	else
		ch->setReadActions(false);
	if (!gui) {
		gSampleChannel *gch = (gSampleChannel*)ch->guiChannel;
		if (gch->readActions) {  
			Fl::lock();
			gch->readActions->value(0);
			Fl::unlock();
		}
	}
}





void glue_quantize(int val) {
	G_Mixer.quantize = val;
	G_Mixer.updateQuanto();
}





void glue_setChanVol(Channel *ch, float v, bool gui) {

	ch->volume = v;

	

	gdEditor *editor = (gdEditor*) gu_getSubwindow(mainWin, WID_SAMPLE_EDITOR);
	if (editor) {
		glue_setVolEditor(editor, (SampleChannel*) ch, v, false);
		Fl::lock();
		editor->volume->value(v);
		Fl::unlock();
	}

	if (!gui) {
		Fl::lock();
		ch->guiChannel->vol->value(v);
		Fl::unlock();
	}
}





void glue_setOutVol(float v, bool gui) {
	G_Mixer.outVol = v;
	if (!gui) {
		Fl::lock();
		mainWin->outVol->value(v);
		Fl::unlock();
	}
}





void glue_setInVol(float v, bool gui) {
	G_Mixer.inVol = v;
	if (!gui) {
		Fl::lock();
		mainWin->inVol->value(v);
		Fl::unlock();
	}
}





void glue_clearAllSamples() {
	G_Mixer.running = false;
	for (unsigned i=0; i<G_Mixer.channels.size; i++) {
		G_Mixer.channels.at(i)->empty();
		G_Mixer.channels.at(i)->guiChannel->reset();
	}
	recorder::init();
	return;
}





void glue_clearAllRecs() {
	recorder::init();
	gu_update_controls();
}





void glue_resetToInitState(bool resetGui) {
	G_Mixer.running = false;
	G_Mixer.ready   = false;
	while (G_Mixer.channels.size > 0)
		G_Mixer.channels.del(0U);  
	mainWin->keyboard->clear();
	recorder::init();
	G_Patch.setDefault();
	G_Mixer.init();
#ifdef WITH_VST
	G_PluginHost.freeAllStacks();
#endif
	if (resetGui)	gu_update_controls();
	G_Mixer.ready = true;
}





void glue_startStopMetronome(bool gui) {
	G_Mixer.metronome = !G_Mixer.metronome;
	if (!gui) {
		Fl::lock();
		mainWin->metronome->value(G_Mixer.metronome);
		Fl::unlock();
	}
}





void glue_setBeginEndChannel(gdEditor *win, SampleChannel *ch, int b, int e, bool recalc, bool check)
{
	if (check) {
		if (e > ch->wave->size)
			e = ch->wave->size;
		if (b < 0)
			b = 0;
		if (b > ch->wave->size)
			b = ch->wave->size-2;
		
		if (b >= ch->end)
			b = ch->begin;
		
		if (e <= ch->begin)
			e = ch->end;
	}

	

	char tmp[16];
	sprintf(tmp, "%d", b/2);
	win->chanStart->value(tmp);

	tmp[0] = '\0';
	sprintf(tmp, "%d", e/2);
	win->chanEnd->value(tmp);

	ch->setBegin(b);
	ch->setEnd(e);

	

	if (recalc) {
		win->waveTools->waveform->recalcPoints();	
		win->waveTools->waveform->redraw();
	}
}





void glue_setBoost(gdEditor *win, SampleChannel *ch, float val, bool numeric) {
	if (numeric) {
		if (val > 20.0f)
			val = 20.0f;
		else if (val < 0.0f)
			val = 0.0f;

	  float linear = pow(10, (val / 20)); 

		ch->boost = linear;

		char buf[10];
		sprintf(buf, "%.2f", val);
		win->boostNum->value(buf);
		win->boostNum->redraw();

		win->boost->value(linear);
		win->boost->redraw();       
	}
	else {
		ch->boost = val;
		char buf[10];
		sprintf(buf, "%.2f", 20*log10(val));
		win->boostNum->value(buf);
		win->boostNum->redraw();
	}
}





void glue_setVolEditor(class gdEditor *win, SampleChannel *ch, float val, bool numeric) {
	if (numeric) {
		if (val > 0.0f)
			val = 0.0f;
		else if (val < -60.0f)
			val = -INFINITY;

	  float linear = pow(10, (val / 20)); 

		ch->volume = linear;

		win->volume->value(linear);
		win->volume->redraw();

		char buf[10];
		if (val > -INFINITY)
			sprintf(buf, "%.2f", val);
		else
			sprintf(buf, "-inf");
		win->volumeNum->value(buf);
		win->volumeNum->redraw();

		ch->guiChannel->vol->value(linear);
		ch->guiChannel->vol->redraw();
	}
	else {
		ch->volume = val;

		float dbVal = 20 * log10(val);
		char buf[10];
		if (dbVal > -INFINITY)
			sprintf(buf, "%.2f", dbVal);
		else
			sprintf(buf, "-inf");

		win->volumeNum->value(buf);
		win->volumeNum->redraw();

		ch->guiChannel->vol->value(val);
		ch->guiChannel->vol->redraw();
	}
}





void glue_setMute(Channel *ch, bool gui) {

	if (recorder::active && recorder::canRec(ch)) {
		if (!ch->mute)
			recorder::startOverdub(ch->index, ACTION_MUTES, G_Mixer.actualFrame);
		else
		 recorder::stopOverdub(G_Mixer.actualFrame);
	}

	ch->mute ? ch->unsetMute(false) : ch->setMute(false);

	if (!gui) {
		Fl::lock();
		ch->guiChannel->mute->value(ch->mute);
		Fl::unlock();
	}
}





void glue_setSoloOn(Channel *ch, bool gui) {

	

	if (!__soloSession__) {
		for (unsigned i=0; i<G_Mixer.channels.size; i++) {
			Channel *och = G_Mixer.channels.at(i);
			och->mute_s  = och->mute;
		}
		__soloSession__ = true;
	}

	ch->solo = !ch->solo;

	

	for (unsigned i=0; i<G_Mixer.channels.size; i++) {
		Channel *och = G_Mixer.channels.at(i);
		if (!och->solo && !och->mute) {
			och->setMute(false);
			Fl::lock();
			och->guiChannel->mute->value(true);
			Fl::unlock();
		}
	}

	if (ch->mute) {
		ch->unsetMute(false);
		Fl::lock();
		ch->guiChannel->mute->value(false);
		Fl::unlock();
	}

	if (!gui) {
		Fl::lock();
		ch->guiChannel->solo->value(1);
		Fl::unlock();
	}
}





void glue_setSoloOff(Channel *ch, bool gui) {

	

	if (mh_uniqueSolo(ch)) {
		__soloSession__ = false;
		for (unsigned i=0; i<G_Mixer.channels.size; i++) {
			Channel *och = G_Mixer.channels.at(i);
			if (och->mute_s) {
				och->setMute(false);
				Fl::lock();
				och->guiChannel->mute->value(true);
				Fl::unlock();
			}
			else {
				och->unsetMute(false);
				Fl::lock();
				och->guiChannel->mute->value(false);
				Fl::unlock();
			}
			och->mute_s = false;
		}
	}
	else {
		ch->setMute(false);
		Fl::lock();
		ch->guiChannel->mute->value(true);
		Fl::unlock();
	}

	ch->solo = !ch->solo;

	if (!gui) {
		Fl::lock();
		ch->guiChannel->solo->value(0);
		Fl::unlock();
	}
}





void glue_setPanning(class gdEditor *win, SampleChannel *ch, float val) {
	if (val < 1.0f) {
		ch->panLeft = 1.0f;
		ch->panRight= 0.0f + val;

		char buf[8];
		sprintf(buf, "%d L", abs((ch->panRight * 100.0f) - 100));
		win->panNum->value(buf);
	}
	else if (val == 1.0f) {
		ch->panLeft = 1.0f;
		ch->panRight= 1.0f;
	  win->panNum->value("C");
	}
	else {
		ch->panLeft = 2.0f - val;
		ch->panRight= 1.0f;

		char buf[8];
		sprintf(buf, "%d R", abs((ch->panLeft * 100.0f) - 100));
		win->panNum->value(buf);
	}
	win->panNum->redraw();
}





void glue_startStopInputRec(bool gui, bool alert) {
	if (G_Mixer.chanInput == NULL) {
		if (!glue_startInputRec(gui)) {
			if (alert) gdAlert("No channels available for recording.");
			else       puts("[glue] no channels available for recording");
		}
	}
	else
		glue_stopInputRec(gui);
}





int glue_startInputRec(bool gui) {

	if (G_audio_status == false)
		return -1;

	SampleChannel *ch = mh_startInputRec();
	if (ch == NULL)	{                  
		Fl::lock();
		mainWin->input_rec->value(0);
		mainWin->input_rec->redraw();
		Fl::unlock();
		return 0;
	}

	if (!G_Mixer.running) {
		glue_startSeq();
		Fl::lock();
		mainWin->beat_stop->value(1);
		mainWin->beat_stop->redraw();
		Fl::unlock();
	}

	glue_setChanVol(ch, 1.0f, false); 

	gu_trim_label(ch->wave->name.c_str(), 28, ch->guiChannel->sampleButton);

	if (!gui) {
		Fl::lock();
		mainWin->input_rec->value(1);
		mainWin->input_rec->redraw();
		Fl::unlock();
	}

	return 1;

}





int glue_stopInputRec(bool gui) {

	SampleChannel *ch = mh_stopInputRec();

	if (ch->mode & (LOOP_BASIC | LOOP_ONCE | LOOP_REPEAT))
		ch->start(0, true);  

	if (!gui) {
		Fl::lock();
		mainWin->input_rec->value(0);
		mainWin->input_rec->redraw();
		Fl::unlock();
	}

	return 1;
}





int glue_saveProject(const char *folderPath, const char *projName) {

	if (gIsProject(folderPath)) {
		puts("[glue] the project folder already exists");
		
	}
	else if (!gMkdir(folderPath)) {
		puts("[glue] unable to make project directory!");
		return 0;
	}

	

	for (unsigned i=0; i<G_Mixer.channels.size; i++) {

		if (G_Mixer.channels.at(i)->type == CHANNEL_SAMPLE) {

			SampleChannel *ch = (SampleChannel*) G_Mixer.channels.at(i);

			if (ch->wave == NULL)
				continue;

			

			char samplePath[PATH_MAX];
			sprintf(samplePath, "%s%s%s.%s", folderPath, gGetSlash().c_str(), ch->wave->basename().c_str(), ch->wave->extension().c_str());

			

			if (gFileExists(samplePath))
				remove(samplePath);
			if (ch->save(samplePath))
				ch->wave->pathfile = samplePath;
		}
	}

	char gptcPath[PATH_MAX];
	sprintf(gptcPath, "%s%s%s.gptc", folderPath, gGetSlash().c_str(), gStripExt(projName).c_str());
	glue_savePatch(gptcPath, projName, true); 

	return 1;
}





void glue_keyPress(Channel *ch, bool ctrl, bool shift) {
	if (ch->type == CHANNEL_SAMPLE)
		glue_keyPress((SampleChannel*)ch, ctrl, shift);
	else
		glue_keyPress((MidiChannel*)ch, ctrl, shift);
}





void glue_keyRelease(Channel *ch, bool ctrl, bool shift) {
	if (ch->type == CHANNEL_SAMPLE)
		glue_keyRelease((SampleChannel*)ch, ctrl, shift);
}





void glue_keyPress(MidiChannel *ch, bool ctrl, bool shift) {
	if (ctrl)
		glue_setMute(ch);
	else
	if (shift)
		ch->kill(0);        
	else
		ch->start(0, true); 
}





void glue_keyPress(SampleChannel *ch, bool ctrl, bool shift) {

	

	if (ctrl)
		glue_setMute(ch);

	

	else
	if (shift) {
		if (recorder::active) {
			if (G_Mixer.running) {
				ch->kill(0); 
				if (recorder::canRec(ch) && !(ch->mode & LOOP_ANY))   
					recorder::rec(ch->index, ACTION_KILLCHAN, G_Mixer.actualFrame);
			}
		}
		else {
			if (ch->hasActions) {
				if (G_Mixer.running || ch->status == STATUS_OFF)
					ch->readActions ? glue_stopReadingRecs(ch) : glue_startReadingRecs(ch);
				else
					ch->kill(0);  
			}
			else
				ch->kill(0);    
		}
	}

	

	else {

		

		if (G_Mixer.quantize == 0 &&
		    recorder::canRec(ch)  &&
	      !(ch->mode & LOOP_ANY))
		{
			if (ch->mode == SINGLE_PRESS)
				recorder::startOverdub(ch->index, ACTION_KEYS, G_Mixer.actualFrame);
			else
				recorder::rec(ch->index, ACTION_KEYPRESS, G_Mixer.actualFrame);
		}

		ch->start(0, true); 
	}

	
}





void glue_keyRelease(SampleChannel *ch, bool ctrl, bool shift) {

	if (!ctrl && !shift) {
		ch->stop();

		

		if (ch->mode == SINGLE_PRESS && recorder::canRec(ch))
			recorder::stopOverdub(G_Mixer.actualFrame);
	}

	

}





void glue_setPitch(class gdEditor *win, SampleChannel *ch, float val, bool numeric)
{
	if (numeric) {
		if (val <= 0.0f)
			val = 0.1000f;
		if (val > 4.0f)
			val = 4.0000f;
		if (win)
			win->pitch->value(val);
	}

	ch->setPitch(val);

	if (win) {
		char buf[16];
		sprintf(buf, "%.4f", val);
		Fl::lock();
		win->pitchNum->value(buf);
		win->pitchNum->redraw();
		Fl::unlock();
	}
}







void glue_beatsMultiply() {
	glue_setBeats(G_Mixer.beats*2, G_Mixer.bars, false);
}

void glue_beatsDivide() {
	glue_setBeats(G_Mixer.beats/2, G_Mixer.bars, false);
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */



#include "graphics.h"

const char *giada_logo_xpm[] = {
"300 82 8 1",
" 	c #181917",
".	c #333432",
"+	c #484A47",
"@	c #5F615E",
"#	c #767875",
"$	c #8D8F8C",
"%	c #A5A7A4",
"&	c #C5C7C4",
"                                                                                                                                                          .#%%&$                                                                                                                                            ",
"                                                                                                                                                      ..#%&&&&&#                                                                                                                                            ",
"                                                                                                                                                 +@#$%&&&&&&&&&@                                                                                                                                            ",
"                                                              ..                                                                                +&&&&&&&&&&&&&&.                                                                                                                                            ",
"                                                            +$%%#+                                                                              +%&&&&&&&&&&&&&.                                                                                                                                            ",
"                                                           #&&&&&%+                                                                             .+@@$&&&&&&&&&%.                                                                                                                                            ",
"                                                          #&&&&&&&$                                                                                  +&&&&&&&&#                                                                                                                                             ",
"                                                         .&&&&&&&&%.                                                                                  #&&&&&&&@                                                                                                                                             ",
"                                                         @&&&&&&&&$                                                                                   +&&&&&&&+                                                                                                                                             ",
"                                                         $&&&&&&&&#                                                                                   .&&&&&&&.                                                                                                                                             ",
"                                                         #&&&&&&&&.                                                                                   +&&&&&&%.                                                                                                                                             ",
"                                                         .&&&&&&&@                                                                                    @&&&&&&$.                                                                                                                                             ",
"                                                          @&&&&&@                                                                                     $&&&&&&#                                                                                                                                              ",
"                                                           .##@.                                                                                      %&&&&&&@                                                                                                                                              ",
"                                                                                                                                                      &&&&&&&+                                                                                                                                              ",
"                                                                                                                                                     .&&&&&&%.                                                                                                                                              ",
"                                                                                                                                                     @&&&&&&$.                                                                                                                                              ",
"                     .+@@###@+.        ......               ...                      ++@@###@@.       .......                      .+@@###@@+.       #&&&&&&#                       .+@@###@@+        .......                                                                                               ",
"                  .@$%%&&&&&&&%$+     +%%%%%%.         .+@#$%%$.                  .@$%&&&&&&&&%$@.    $%%%%%%+                  .@#%%&&&&&&&&%$@     %&&&&&&@                    .@$%%&&&&&&&%$#.    @%%%%%%@                                                                                               ",
"                 #%&&&&&&&&&&&&&&$.   #&&&&&&    +@#$$%%%&&&&&%.                .$%&&&&&&&&&&&&&&%@  .&&&&&&&+                 #%&&&&&&&&&&&&&&&$+   &&&&&&&@                   #%&&&&&&&&&&&&&&%#.  %&&&&&&@                                                                                               ",
"               +%&&&&&&&&&&&&&&&&&%@ .&&&&&&%   .%&&&&&&&&&&&&$                #%&&&&&&&&&&&&&&&&&&$ +&&&&&&%.               @%&&&&&&&&&&&&&&&&&&&# .&&&&&&%+                 @%&&&&&&&&&&&&&&&&&&%. &&&&&&&+                                                                                               ",
"              #&&&&&&&&&#+....@$&&&&@@&&&&&&$   .&&&&&&&&&&&&&#              +%&&&&&&&&%#+....+#%&&&$#&&&&&&$.             .$&&&&&&&&&$+.....@%&&&&$@&&&&&&%.               .$&&&&&&&&&#@.....#%&&&%+&&&&&&%.                                                                                               ",
"             $&&&&&&&&@         .%&&&&&&&&&&@    .@$&&&&&&&&&&@             +%&&&&&&&%@         .#&&&&&&&&&&$             .%&&&&&&&&@          .$&&&&&&&&&&$               .%&&&&&&&&#.         @&&&&&&&&&&%.                                                                                               ",
"            $&&&&&&&%.            @&&&&&&&&&+       .%&&&&&&&%+            @&&&&&&&&#             +%&&&&&&&&#            +%&&&&&&&$.             @&&&&&&&&&#              .%&&&&&&&$.            .%&&&&&&&&$                                                                                                ",
"           %&&&&&&&#               @&&&&&&&&.        +%&&&&&&%.           @&&&&&&&&#               .%&&&&&&&@           +%&&&&&&&#                @&&&&&&&&@             +%&&&&&&&#                %&&&&&&&@                                                                                                ",
"          $&&&&&&&$                 #&&&&&&%.         %&&&&&&%           +&&&&&&&&@                 +&&&&&&%+          .%&&&&&&&#                  @&&&&&&&+            .%&&&&&&&#                  &&&&&&&+                                                                                                ",
"         @&&&&&&&$.                 #&&&&&&$          %&&&&&&$          .%&&&&&&&@                  @&&&&&&%.         .$&&&&&&&$                   +&&&&&&%+            $&&&&&&&$                  .&&&&&&&+                                                                                                ",
"        +&&&&&&&%+                  %&&&&&&#          %&&&&&&#          $&&&&&&&$                   $&&&&&&%          @&&&&&&&%                    #&&&&&&%.           #&&&&&&&%.                  @&&&&&&%.                                                                                                ",
"        %&&&&&&&#                   &&&&&&&+         .%&&&&&&@         @&&&&&&&$                    %&&&&&&$         +&&&&&&&&+                    $&&&&&&$           +&&&&&&&%.                   $&&&&&&$.                                                                                                ",
"       @&&&&&&&%.                  .&&&&&&&.         +%&&&&&%.        .&&&&&&&&.                   .%&&&&&&#         $&&&&&&&#                     %&&&&&&#          .$&&&&&&&@                    %&&&&&&#                                                                                                 ",
"      .%&&&&&&&@                   @&&&&&&&.         @&&&&&&%         @&&&&&&&#                    @&&&&&&&+        +&&&&&&&&.                    +&&&&&&&@          +&&&&&&&%                    .&&&&&&&@                                                                                                 ",
"      @&&&&&&&%                    $&&&&&&%.         #&&&&&&%         &&&&&&&&.                    #&&&&&&%.        %&&&&&&&#                     @&&&&&&%+         .$&&&&&&&@                    +&&&&&&&+                                                                                                 ",
"     .$&&&&&&&#                    %&&&&&&#          $&&&&&&#        #&&&&&&&#                     $&&&&&&%        +&&&&&&&&.                     $&&&&&&%.         +&&&&&&&%                     #&&&&&&%+                                                                                                 ",
"     +%&&&&&&&+                   .&&&&&&&@         .%&&&&&&@        %&&&&&&&+                    .%&&&&&&$        $&&&&&&&$                      %&&&&&&%          #&&&&&&&#                     %&&&&&&%.                                                                                                 ",
"     @&&&&&&&%                    +&&&&&&&+         +%&&&&&&+       .&&&&&&&%.                    +&&&&&&&#        &&&&&&&&+                     +%&&&&&&$         .%&&&&&&&.                     &&&&&&&$                                                                                                  ",
"     $&&&&&&&@                    #&&&&&&&.         @&&&&&&&.       #&&&&&&&#                     #&&&&&&&@       +&&&&&&&%.                     @&&&&&&&#         .&&&&&&&%                     +&&&&&&&#                                                                                                  ",
"    .%&&&&&&&.                    %&&&&&&%.         #&&&&&&%        %&&&&&&&+                     $&&&&&&&+       #&&&&&&&$.                     $&&&&&&&+         @&&&&&&&@                     #&&&&&&&@                                                                                                  ",
"    +&&&&&&&&                     &&&&&&&$.         #&&&&&&$        &&&&&&&%.                    .%&&&&&&&        %&&&&&&&#                     .%&&&&&&%.         $&&&&&&&+                     $&&&&&&%+                                                                                                  ",
"    +&&&&&&&$                    +&&&&&&&#         .$&&&&&&#       .&&&&&&&$.                    +&&&&&&&%        %&&&&&&&+                     +%&&&&&&%          %&&&&&&&.                    .%&&&&&&%.                                                                                                  ",
"    @&&&&&&&@                    $&&&&&&&@         .%&&&&&&+       +&&&&&&&#                     #&&&&&&&$        &&&&&&&&+                     #&&&&&&&%          &&&&&&&%                     @&&&&&&&$                                                                                                   ",
"    @&&&&&&&+                    &&&&&&&&+         +%&&&&&&.       @&&&&&&&@                    .%&&&&&&&@       .&&&&&&&%.                    .%&&&&&&&#          &&&&&&&#                     $&&&&&&&$                                                                                                   ",
"    #&&&&&&&.                   @&&&&&&&%.         @&&&&&&&        @&&&&&&&@                    @&&&&&&&&+       .&&&&&&&%.                    @&&&&&&&&@         .&&&&&&&#                    +%&&&&&&&#                                                                                                   ",
"    #&&&&&&&.                   %&&&&&&&$.         #&&&&&&%        #&&&&&&&+                   .$&&&&&&&&.       .&&&&&&&$.                   .$&&&&&&&&.         .&&&&&&&@                    $&&&&&&&&@                                                                                                   ",
"    #&&&&&&&                   #&&&&&&&&$          $&&&&&&#        @&&&&&&&+                   @&&&&&&&&&        .&&&&&&&$.                   @&&&&&&&&&          .&&&&&&&+                   +%&&&&&&&%+                                                                                                   ",
"    @&&&&&&&                  .%&&&&&&&&#         .%&&&&&&@        @&&&&&&%+                  .%&&&&&&&&%         &&&&&&&$.                  .%&&&&&&&&%           &&&&&&&+                   $&&&&&&&&%.                                                                                                   ",
"    @&&&&&&&.                 $&&&&&&&&&@         +%&&&&&&.        +&&&&&&&@                  @&&&&&&&&&$         &&&&&&&%.                  #&&&&&&&&&$           &&&&&&&@                  +&&&&&&&&&%                                                                                                    ",
"    +&&&&&&&+                @&&&&&&&&&%+         +&&&&&&&          &&&&&&&@                 +&&&&&&&&&&#         $&&&&&&%+                 @&&&&&&&&&&#           $&&&&&&#                 .%&&&&&&&&&%                                                                                                    ",
"    .%&&&&&&@               +%&&$&&&&&&%.         +&&&&&&&          %&&&&&&#                .&&&&&&&&&&&@         @&&&&&&&+                +&&&$%&&&&&&@           @&&&&&&$                .$&&&&&&&&&&$                                                                                                    ",
"     #&&&&&&$              .$&&##&&&&&&$          @&&&&&&&          @&&&&&&%.              .%&&%@%&&&&&&@         .&&&&&&&#               .&&&$+%&&&&&&.           .&&&&&&&.              .#&&%@%&&&&&&$                                                                                                    ",
"     +&&&&&&&.            .%&&% $&&&&&&#          +&&&&&&&.        +.&&&&&&&@             .%&&%+.%&&&&&&$        @+$&&&&&&&+             +&&&% +&&&&&&&+        .+  $&&&&&&$             .$&&&@ %&&&&&&%        .#                                                                                          ",
"      $&&&&&&$           +%&&&+ %&&&&&&@          +&&&&&&&@       #&$#&&&&&&%+           @&&&&@ .$&&&&&&&+      #&&@&&&&&&&$.          .#&&&%. +%&&&&&&#       .$&$ +&&&&&&&+           +%&&&#  $&&&&&&&#      +&&$                                                                                         ",
"      +%&&&&&&#.       +#&&&%@ .%&&&&&&+          .%&&&&&&&+    .$&&%.%&&&&&&%+        +#&&&&@   #&&&&&&&%@..+@$&&&+@&&&&&&&%+       .@%&&&%+  .%&&&&&&&+     +%&&$. #&&&&&&%@        +#&&&&#   @&&&&&&&&@...+#&&&#                                                                                         ",
"       @&&&&&&&$@+..++#%&&&%+  +&&&&&&%.           $&&&&&&&%#@@#%&&%. .%&&&&&&%@+...+@$&&&&%+    +%&&&&&&&&%$%&&&&+  $&&&&&&&&$#@+@@#%&&&&$.    #&&&&&&&&#@+@$&&&$.  .$&&&&&&&#+...++#%&&&&@    .%&&&&&&&&%$%&&&&#                                                                                          ",
"        @&&&&&&&&%%%%&&&&&$.   @&&&&&&%            +%&&&&&&&&&&&&&$.   +%&&&&&&&&%$%%&&&&&$.      #&&&&&&&&&&&&&%+   .$&&&&&&&&&&&&&&&&&&#      .%&&&&&&&&&&&&&&#     .$&&&&&&&&%$%%&&&&&%+      @&&&&&&&&&&&&&%@                                                                                           ",
"         @%&&&&&&&&&&&&&%@.    #&&&&&&$             #&&&&&&&&&&&%@.     .$&&&&&&&&&&&&&&%@.       .%&&&&&&&&&&&#.      @&&&&&&&&&&&&&&&$+        +&&&&&&&&&&&&%+       .#&&&&&&&&&&&&&&&#.        $&&&&&&&&&&&$+                                                                                            ",
"          .#%&&&&&&&&&%+.      %&&&&&&#              +%&&&&&&&%#.         +$&&&&&&&&&&%@.          .$&&&&&&&&#+         .#&&&&&&&&&&%#.           .$&&&&&&&&%@.          +$&&&&&&&&&&%@.          .@&&&&&&&&$+.                                                                                             ",
"            .+#$%%$#+..       .%&&&&&&+               .@#$%$#+.             .@#$%%$#@+               +@$%$#+.             .+#$%%$#@+.               +@$%$$@.               .+#$%%$#@+.              .@$%$#@.                                                                                                ",
"                              +&&&&&&%.                                                                                                                                                                                                                                                                     ",
"                              #&&&&&&%                                                                                                                                                                                                                                                                      ",
"                             .%&&&&&&@                                                                                                                                                                                                                                                                      ",
"                             @&&&&&&%.                                                                                                                                                                                                                                                                      ",
"                             $&&&&&&$                                                                                                                                                                                                                                                                       ",
"                            @&&&&&&&+                                                                                                                                                                                                                                                                       ",
"  @#$#+                    .$&&&&&&$                                                                                                                                                                                                                                                                        ",
" $&&&&&#                   #&&&&&&%                                                                                                                                                                                                                                                                         ",
"#&&&&&&&@                 @&&&&&&&+                                                                                                                                                                                                                                                                         ",
"%&&&&&&&%.               @&&&&&&%+                                                                                                                                                                                                                                                                          ",
"%&&&&&&&&#              #&&&&&&%.                                                                                                                                                                                                                                                                           ",
"@&&&&&&&&&@           .$&&&&&&#.                                                                                                                                                                                                                                                                            ",
" $&&&&&&&&&$+       +$&&&&&&%+                                                                                                                                                                                                                                                                              ",
"  +&&&&&&&&&&%#@@#$%&&&&&&&#.                      +.     .+    +@.     ++    .+   +++++          +.    .+     ++     +++++.    .++++.        +@.       .@+     .++++.    .+++++++       +.         +@.       .+@.    .++++.         ++.      ++.    .+.      .+@.    .+     +.  .+  .+.    .+  .+++++++    ",
"   .$&&&&&&&&&&&&&&&&&&&%@                         $%.    %@  +&&%&%.  .$$    +&  .&&&&&&#       .%@    +&.    %&+    $&&&&&%.  @&&&&&%.    +&&%&%.   .%&%&&+   #&&&&&&@  +&&&&&&%.      &#       +&&%&%.    @&&%&%.  +&&&&&&@       $&%     +%&+   .$&@     @&&%&$.  $%    .%$  $%  +&%.   +%. +&&&&&&$.   ",
"      +$%%&&&&&&&&&&%$@.                           +&#   #%  +&#   %%   $$    +&  .&+   @&@      .%@    +&.   +$&$    $$   .%%  @%   .%&.  .&$  .%$.  %%.  #&+  #&   .$&+ +&.            &#      +&#   $%.  @&#  .%%. +&.   $&+      $%&+    #%&+   .&%$    @&#  +%$  $%    .%#  $%  +&%$   +%. +&.         ",
"         .++@###@@+.                                #&. .&+  $%.   .&@  $$    +&  .&+    %$      .%@    +&.   ##$%    $$    @&. @%     %$  $%.   @&. @&+    %$. #&    .%@ +&.            &#      %%    .&#  %%    .&@ +&.    &#      $#%$    $#&+   @%@%.   %%    @%+ $%    .%#  $%  +&+%+  +%. +&.         ",
"                                                    .%$ $$  .&#     %%  $$    +&  .&+    %$      .%@    +&.  .%@+&+   $$    @&. @%     #&  &#     +. %%     #&. #&    .%@ +&.            &#     .&@     $$ +&+    .$$ +&.    %#      $#@&   +$@&+   $#.%@  +&@    .+. $%    .%#  $%  +& $$  +%. +&.         ",
"                                                     @&#&.  .&@     $&  $$    +&  .&#+++#&+      .%%####$&.  +%. %$   $%.++@&$  @%     +&..&@        &$     @&+ #&++++$%. +&$###@        &#     +&+     #%.@&.     #% +&+ ..#&+      $#.&@  ##+&+  .&..$$  @&.        $&$###$&#  $%  +& +%@ +%. +&####+     ",
"                                                      %&@   +&+     #&  $$    +&  .&%$$%%@       .%%####$&.  #$  #&   $&$$%&#.  @%     +&++&@        &#     +&+ #&$$%&%+  +&$$$$#        &#     @&.     #%.@&.     #%.+%%%%%%@       $# $% .$+.&+  @%  @%. @&.        $&$###$&#  $%  +&  #%.+%. +&$$$$@     ",
"                                                      #&.   .&@     $%  $$    +&  .&#+.$%.       .%@    +&. .$%##$&+  $%.+@&@   @%     +&..&@        &$     @&. #&+++$$   +&+            &#     +&.     #% @&.     $$ +&#@@++        $# +&.+$.+&+  $%@#$&+ @&.        $%    .%#  $%  +&  .%@+%. +&.         ",
"                                                      @&    .%#     &$  $$    +%  .&+  .%@       .%@    +&. +%$###&$  $$   $%   @%     $&  &$    .$+ $%     #%. #&   +&+  +&.            &#     .&@    .%$ +&@    .%# +&.            $# .%### +&+ .&####&# .&@    .$+ $%    .%#  $%  +&   @%@%. +&.         ",
"                                                      @&     #%.   @&.  #%.   $%  .&+   $%       .%@    +&. @$.   #&  $$   +&+  @%    +&@  #&.   #&. +&+   .%@  #&   .%#  +&.            &#      $%    +&+  %%    @&+ +&.            $#  #&%+ +&+ @%    #%. %%    #%. $%    .%#  $%  +&   .$%%. +&.         ",
"                                                      @&     .%%+.@&#   .%%++#&@  .&+   +&+      .%@    +&. $#    .&+ $$    %$  @&+++#&%.   $%+.#&#   #&@.+%%.  #&    @%. +&@+++++.      &$++++. .%%+.@%#   .%%..@&#  +&.            $#  +&$  +&+ %#    +&+ .%%+.#&#  $%    .%#  $%  +&    +&&. +&@++++.  %&",
"                                                      @&      .$&&%@     +%&&&@   .&+    %$      .%@    +&..%+     %$ $$    @&. @&&&&%@     .$%&&#     @%&&$.   #&    .%@ +&&&&&&%+      &&&&&&$. .$&&%#     .$&&%@   +&.            $#  .%#  +&+.&.    .%#  .$&&&@   $%    .%#  $%  +&     $&. +&&&&&&%. &&"};


const char *loopRepeat_xpm[] = {
"18 18 8 1",
" 	c #181917",
".	c #242523",
"+	c #323331",
"@	c #4D4F4C",
"#	c #646663",
"$	c #787A77",
"%	c #919390",
"&	c #BFC1BD",
"..................",
"..................",
"..................",
"...&%#......#%&...",
"...&&&%+..+%&&&...",
"...$%&&%..%&&%$...",
".....$&&##&&$.....",
"......%&%%&%......",
"......$&&&&$......",
"......$&&&&$......",
"......%&%%&%......",
".....$&&##&&$.....",
"...$%&&%..%&&%$...",
"...&&&%+..+%&&&...",
"...&%#......#%&...",
"..................",
"..................",
".................."};


const char *loopBasic_xpm[] = {
"18 18 8 1",
" 	c #181917",
".	c #242523",
"+	c #313230",
"@	c #4D4F4C",
"#	c #666765",
"$	c #787A77",
"%	c #919390",
"&	c #BEC0BD",
"..................",
"..................",
"..................",
"......#%&&%#......",
"....+%&&&&&&%+....",
"....%&&&%%&&&%....",
"...#&&%+..+%&&#...",
"...%&&+....+&&%...",
"...&&%......%&&...",
"...&&%......%&&...",
"...%&&+....+&&%...",
"...#&&%+..+%&&#...",
"....%&&&%%&&&%....",
"....+%&&&&&&%+....",
"......#%&&%#......",
"..................",
"..................",
".................."};


const char *loopOnce_xpm[] = {
"18 18 8 1",
" 	c #181917",
".	c #242523",
"+	c #323331",
"@	c #4D4F4C",
"#	c #646663",
"$	c #787A77",
"%	c #919390",
"&	c #BFC1BD",
"..................",
"..................",
"..................",
"......$%&&%#......",
"....+&&&&&&&&+....",
"...+&&&&$$&&&&+...",
"...$&&$....$&&$...",
"...%&&......%&%...",
"..................",
"..................",
"...%&&+.....&&&...",
"...#&&$....$&&#...",
"....%&&&%$&&&%....",
"....+%&&&&&&%+....",
"......#%&&%#......",
"..................",
"..................",
".................."};


const char *oneshotBasic_xpm[] = {
"18 18 8 1",
" 	c #181917",
".	c #242523",
"+	c #313230",
"@	c #4D4F4C",
"#	c #666765",
"$	c #787A77",
"%	c #919390",
"&	c #BEC0BD",
"..................",
"..................",
"..................",
"..................",
"..................",
"..................",
"..................",
"..................",
"..................",
"..................",
"..................",
"..................",
"...$$$$$$$$$$$$...",
"...&&&&&&&&&&&&...",
"...&&&&&&&&&&&&...",
"..................",
"..................",
".................."};


const char *oneshotRetrig_xpm[] = {
"18 18 8 1",
" 	c #181917",
".	c #242523",
"+	c #313230",
"@	c #4D4F4C",
"#	c #666765",
"$	c #787A77",
"%	c #919390",
"&	c #BEC0BD",
"..................",
"..................",
"..................",
"..................",
"..................",
"...$$$$$$$#@......",
"...&&&&&&&&&&@....",
"...&&&&&&&&&&&+...",
"..........+$&&%...",
"............%&&...",
"............%&&...",
"...........+&&%...",
"...$$$$$$$%&&&#...",
"...&&&&&&&&&&%....",
"...&&&&&&&&%#.....",
"..................",
"..................",
".................."};


const char *oneshotPress_xpm[] = {
"18 18 8 1",
" 	c #181917",
".	c #242523",
"+	c #313230",
"@	c #4D4F4C",
"#	c #666765",
"$	c #787A77",
"%	c #919390",
"&	c #BEC0BD",
"..................",
"..................",
"..................",
"..................",
"..................",
"..................",
"...+%&%+..........",
"...%&&&%..........",
"...&&&&&..........",
"...$&&&$..........",
"...+$&$+..........",
"..................",
"...$$$$$$$$$$$$...",
"...&&&&&&&&&&&&...",
"...&&&&&&&&&&&&...",
"..................",
"..................",
".................."};


const char *oneshotEndless_xpm[] = {
"18 18 6 1",
" 	c #242523",
".	c #464745",
"+	c #6D6F6C",
"@	c #888A87",
"#	c #ADAFAC",
"$	c #C6C8C5",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"        .++.      ",
"       @$$$$#.    ",
"      @$$$$$$$.   ",
"     .$$#. +$$@   ",
"     +$$.   @$#   ",
"     +$$    @$$   ",
"     .$$+   #$#   ",
"   @@@$$$@@#$$+   ",
"   $$$$$$$$$$@    ",
"   $$$$$$$$#+     ",
"                  ",
"                  ",
"                  "};


const char *updirOff_xpm[] = {
"18 18 8 1",
" 	c #242523",
".	c #332F2E",
"+	c #54494A",
"@	c #6B5A5C",
"#	c #866C6B",
"$	c #967B7A",
"%	c #987D7C",
"&	c #B18E8F",
"                  ",
"                  ",
"                  ",
"                  ",
"        @@        ",
"       #&&#       ",
"     .#&&&&#.     ",
"    .$&&&&&&$.    ",
"    +@%&&&&%@+    ",
"      #&&&&#      ",
"      #&&&&#      ",
"      #&&&&#      ",
"      #&&&&#      ",
"      #&&&&#      ",
"       ....       ",
"                  ",
"                  ",
"                  "};


const char *updirOn_xpm[] = {
"18 18 8 1",
" 	c #4D4F4C",
".	c #555150",
"+	c #706465",
"@	c #7D6B6E",
"#	c #877373",
"$	c #957978",
"%	c #9F8382",
"&	c #B18E8F",
"                  ",
"                  ",
"                  ",
"                  ",
"        ##        ",
"       #&&#       ",
"     .$&&&&$.     ",
"    .%&&&&&&%.    ",
"    +@%&&&&%@+    ",
"      $&&&&$      ",
"      $&&&&$      ",
"      $&&&&$      ",
"      $&&&&$      ",
"      $&&&&$      ",
"       ....       ",
"                  ",
"                  ",
"                  "};


const char *pause_xpm[] = {
"23 23 8 1",
" 	c #4D4F4C",
".	c #514E53",
"+	c #5C4F61",
"@	c #6F507E",
"#	c #855098",
"$	c #9551AE",
"%	c #A652C5",
"&	c #AE52D1",
"                       ",
"                       ",
"                       ",
"                       ",
"                       ",
"       #+              ",
"       &%#.            ",
"       &&&%@           ",
"       &&&&&$+         ",
"       &&&&&&&#.       ",
"       &&&&&&&&%@      ",
"       &&&&&&&&&&#     ",
"       &&&&&&&&%@.     ",
"       &&&&&&&#.       ",
"       &&&&&$+         ",
"       &&&%@           ",
"       &&#.            ",
"       $+              ",
"                       ",
"                       ",
"                       ",
"                       ",
"                       "};


const char *play_xpm[] = {
"23 23 8 1",
" 	c #242523",
".	c #393534",
"+	c #574B4C",
"@	c #6E5B5A",
"#	c #7C6663",
"$	c #8C7170",
"%	c #A48384",
"&	c #B18E8F",
"                       ",
"                       ",
"                       ",
"                       ",
"                       ",
"       $.              ",
"       &&@             ",
"       &&&%+           ",
"       &&&&&$.         ",
"       &&&&&&&@        ",
"       &&&&&&&&%+      ",
"       &&&&&&&&&&#     ",
"       &&&&&&&&%+      ",
"       &&&&&&&#.       ",
"       &&&&&$.         ",
"       &&&%+           ",
"       &&@             ",
"       $.              ",
"                       ",
"                       ",
"                       ",
"                       ",
"                       "};


const char *rewindOff_xpm[] = {
"23 23 8 1",
" 	c #242523",
".	c #393534",
"+	c #574B4C",
"@	c #6E5B5A",
"#	c #7C6663",
"$	c #8C7170",
"%	c #A48384",
"&	c #B18E8F",
"                       ",
"                       ",
"                       ",
"                       ",
"                       ",
"                .$     ",
"               @&&     ",
"             +%&&&     ",
"           .$&&&&&     ",
"          @&&&&&&&     ",
"        +%&&&&&&&&     ",
"       #&&&&&&&&&&     ",
"        +%&&&&&&&&     ",
"         .#&&&&&&&     ",
"           .$&&&&&     ",
"             +%&&&     ",
"               @&&     ",
"                .$     ",
"                       ",
"                       ",
"                       ",
"                       ",
"                       "};


const char *rewindOn_xpm[] = {
"23 23 8 1",
" 	c #4D4F4C",
".	c #514E53",
"+	c #5C4F61",
"@	c #6F507E",
"#	c #855098",
"$	c #9551AE",
"%	c #A652C5",
"&	c #AE52D1",
"                       ",
"                       ",
"                       ",
"                       ",
"                       ",
"                +#     ",
"              .#%&     ",
"             @%&&&     ",
"           +$&&&&&     ",
"         .#&&&&&&&     ",
"        @%&&&&&&&&     ",
"       #&&&&&&&&&&     ",
"       .@%&&&&&&&&     ",
"         .#&&&&&&&     ",
"           +$&&&&&     ",
"             @%&&&     ",
"              .#&&     ",
"                +$     ",
"                       ",
"                       ",
"                       ",
"                       ",
"                       "};





const char *giada_icon[] = {
"65 65 11 1",
" 	c None",
".	c #000000",
"+	c #000100",
"@	c #FFFFFF",
"#	c #FDFFFC",
"$	c #CBCDC9",
"%	c #292B28",
"&	c #626461",
"*	c #484A47",
"=	c #888A87",
"-	c #A7A9A6",
"....+++++++++++++++++++++++++++++++++++++++++++++++++++++++++....",
".@@@#####################$%+++++++++++++%&&*%+++++++&##%+++++....",
".@@#####################&++++++++++++=$#######-*+++++&$+++++++...",
".@@####################&++++++++++%-############$*+++++++++++++..",
"+#####################*++++++++++&################-++++++++++++%+",
"+####################*++++++++++=##################$+++++++++++*+",
"+###################*++++++++++$####################$++++++++++=+",
"+##################=++++++++++-######################-+++++++++-+",
"+#################$++++++++++=#######################=+++++++++$+",
"+#################%+++++++++*########################*+++++++++#+",
"+################*++++++++++#########################%++++++++*#+",
"+###############-++++++++++=########################$+++++++++&#+",
"+###############%+++++++++%#########################-+++++++++-#+",
"+##############-++++++++++-#########################&+++++++++$#+",
"+##############%+++++++++%##########################*+++++++++##+",
"+#############-++++++++++-##########################+++++++++%##+",
"+#############%++++++++++##########################$+++++++++*##+",
"+############$++++++++++&##########################=+++++++++=##+",
"+############=++++++++++$##########################&+++++++++-##+",
"+############*+++++++++%###########################%+++++++++$##+",
"+############++++++++++=###########################++++++++++###+",
"+###########$++++++++++$##########################-+++++++++*###+",
"+###########=++++++++++###########################=+++++++++&###+",
"+###########*+++++++++%###########################*+++++++++-###+",
"+###########%+++++++++&###########################++++++++++$###+",
"+###########%+++++++++-##########################=++++++++++####+",
"+###########++++++++++$##########################%+++++++++%####+",
"+###########++++++++++##########################$++++++++++&####+",
"+##########$++++++++++##########################&++++++++++=####+",
"+##########$+++++++++%#########################$+++++++++++-####+",
"+##########$+++++++++%#########################&+++++++++++$####+",
"+###########+++++++++*########################$++++++++++++#####+",
"+###########+++++++++*########################*+++++++++++*#####+",
"+###########%++++++++%#######################=++++++++++++&#####+",
"+###########&+++++++++######################$+++++++++++++-#####+",
"+###########=+++++++++$#####################%+++%+++++++++$#####+",
"+###########$+++++++++$####################&+++%=+++++++++######+",
"+############%++++++++&###################=++++$&++++++++%######+",
"+############-+++++++++$#################&++++=#*++++++++&######+",
"+#############+++++++++&################*++++*##+++++++++=######+",
"+#############=+++++++++-#############$%++++%###+++++++++-######+",
"+##############*+++++++++=##########$*+++++%###$+++++++++#######+",
"+###############++++++++++&$######$&++++++&####=+++++++++#######+",
"+###############$++++++++++++%&*%++++++++=#####&++++++++*#######+",
"+################$%+++++++++++++++++++++-######%++++++++=#######+",
"+##################&++++++++++++++++++=########+++++++++-#######+",
"+###################$%+++++++++++++%=#########$+++++++++########+",
"+#####################$=%++++++%*=-###########=++++++++%########+",
"+##########################$$$################*++++++++&########+",
"+#############################################+++++++++=########+",
"+############################################$+++++++++$########+",
"+############################################&++++++++%#########+",
"+############################################+++++++++=#########+",
"+###########################################=+++++++++##########+",
"+###########################################%++++++++*##########+",
"+##########################################=+++++++++-##########+",
"+#########-==$############################$+++++++++&###########+",
"+#######=++++++-##########################*+++++++++############+",
"+######$++++++++$########################=+++++++++-############+",
"+######=+++++++++$######################-+++++++++&#############+",
"+######=+++++++++*#####################$+++++++++&##############.",
".@#####=++++++++++-###################-+++++++++=##############@.",
".@@####=++++++++++%##################=+++++++++=#############@@@.",
".@@@###=+++++++++++*###############$*++++++++%$##############@@@.",
"....++++++++++++++++++++++++++++++++++++++++++++++++++++++++....."};

const char *recOff_xpm[] = {
"23 23 8 1",
" 	c #242523",
".	c #342F2E",
"+	c #3F3B3A",
"@	c #594F4F",
"#	c #7A6663",
"$	c #8C7170",
"%	c #A68384",
"&	c #B18E8F",
"                       ",
"                       ",
"                       ",
"                       ",
"                       ",
"         @$%%$@        ",
"       .$&&&&&&$.      ",
"       $&&&&&&&&$      ",
"      @&&&#++#&&&@     ",
"      $&&#    #&&$     ",
"      %&&+    +&&%     ",
"      %&&+    +&&%     ",
"      $&&#    #&&$     ",
"      @&&&#++#&&&@     ",
"       $&&&&&&&&$      ",
"       .$&&&&&&$.      ",
"         @$%%$@        ",
"                       ",
"                       ",
"                       ",
"                       ",
"                       ",
"                       "};

const char *recOn_xpm[] = {
"23 23 8 1",
" 	c #4D4F4C",
".	c #5F4E50",
"+	c #6E4F50",
"@	c #8C5050",
"#	c #AE5454",
"$	c #BB5253",
"%	c #C55352",
"&	c #E85557",
"                       ",
"                       ",
"                       ",
"                       ",
"                       ",
"         @$&&$@        ",
"       .%&&&&&&%.      ",
"       %&&&&&&&&%      ",
"      @&&&#++#&&&@     ",
"      $&&#    #&&$     ",
"      &&&+    +&&&     ",
"      &&&+    +&&&     ",
"      $&&#    #&&$     ",
"      @&&&#++#&&&@     ",
"       %&&&&&&&&%      ",
"       .%&&&&&&%.      ",
"         @$&&$@        ",
"                       ",
"                       ",
"                       ",
"                       ",
"                       ",
"                       "};

const char *inputRecOn_xpm[] = {
"23 23 8 1",
" 	c #524D4C",
".	c #4D4F4C",
"+	c #5D4F50",
"@	c #8C5050",
"#	c #BB5253",
"$	c #C45251",
"%	c #DD5256",
"&	c #EA5657",
".......................",
".......................",
".......................",
".......................",
".......................",
"........ @#%%#@ .......",
".......+$&&&&&&$+......",
"...... $&&&&&&&&$ .....",
"......@&&&&&&&&&&@.....",
"......#&&&&&&&&&&#.....",
"......%&&&&&&&&&&%.....",
"......%&&&&&&&&&&%.....",
"......#&&&&&&&&&&#.....",
"......@&&&&&&&&&&@.....",
".......$&&&&&&&&$......",
".......+$&&&&&&$+......",
"........ @#%%#@ .......",
".......................",
".......................",
".......................",
".......................",
".......................",
"......................."};

const char *inputRecOff_xpm[] = {
"23 23 8 1",
" 	c #242523",
".	c #252724",
"+	c #332F2E",
"@	c #594E4F",
"#	c #896E6D",
"$	c #8D7271",
"%	c #A68384",
"&	c #B18E8F",
"                       ",
"                       ",
"                       ",
"                       ",
"                       ",
"        .@#%%#@.       ",
"       +$&&&&&&$+      ",
"      .$&&&&&&&&$.     ",
"      @&&&&&&&&&&@     ",
"      #&&&&&&&&&&#     ",
"      %&&&&&&&&&&%     ",
"      %&&&&&&&&&&%     ",
"      #&&&&&&&&&&#     ",
"      @&&&&&&&&&&@     ",
"       $&&&&&&&&$      ",
"       +$&&&&&&$+      ",
"        .@#%%#@.       ",
"                       ",
"                       ",
"                       ",
"                       ",
"                       ",
"                       "};

const char *muteOff_xpm[] = {
"18 18 8 1",
" 	c #242523",
".	c #2E2F2D",
"+	c #3B3C3A",
"@	c #525451",
"#	c #6F716E",
"$	c #878986",
"%	c #ADAFAC",
"&	c #C6C8C5",
"                  ",
"                  ",
"                  ",
"                  ",
"     ++.  .++     ",
"    +&&$  $&&+    ",
"    +&&%  %&&+    ",
"    +&%&++&%&+    ",
"    +&$&##&$&+    ",
"    +&#%$$%#&+    ",
"    +&#$%%$#&+    ",
"    +&#@&&@#&+    ",
"    +&#+&&+#&+    ",
"    .#@ ## @#.    ",
"                  ",
"                  ",
"                  ",
"                  "};

const char *muteOn_xpm[] = {
"18 18 8 1",
" 	c #4D4F4C",
".	c #585A57",
"+	c #616260",
"@	c #7A7C79",
"#	c #888A87",
"$	c #989A97",
"%	c #B2B4B1",
"&	c #C6C8C5",
"                  ",
"                  ",
"                  ",
"                  ",
"     ..    ..     ",
"    +&&$  $&&+    ",
"    +&&%  %&&+    ",
"    +&%&++&%&+    ",
"    +&$&@@&$&+    ",
"    +&#%$$%#&+    ",
"    +&#$&&$#&+    ",
"    +&#@&&@#&+    ",
"    +&#.&&.#&+    ",
"    .#+ ## +#.    ",
"                  ",
"                  ",
"                  ",
"                  "};


const char *readActionOff_xpm[] = {
"18 18 8 1",
" 	c #242523",
".	c #393B38",
"+	c #555754",
"@	c #6B6D6A",
"#	c #7F807E",
"$	c #9C9E9B",
"%	c #B1B3B0",
"&	c #C3C5C2",
"                  ",
"                  ",
"                  ",
"                  ",
"     ....         ",
"     %&&&&%+      ",
"     %&@@@&&      ",
"     %%   $&.     ",
"     %&@@#&$      ",
"     %&&&&@       ",
"     %% +&$       ",
"     %%  #&#      ",
"     %%   %&+     ",
"     @@   .#+     ",
"                  ",
"                  ",
"                  ",
"                  "};


const char *readActionOn_xpm[] = {
"18 18 8 1",
" 	c #4D4F4C",
".	c #696B68",
"+	c #7A7C79",
"@	c #888A87",
"#	c #939592",
"$	c #A7A9A6",
"%	c #B7B9B6",
"&	c #C4C6C3",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"     %&&&&%.      ",
"     %&++@&&      ",
"     %%   $&      ",
"     %&@@#&$      ",
"     %&&&&@       ",
"     %% +&$       ",
"     %%  #&#      ",
"     %%   %&.     ",
"     +@   .@+     ",
"                  ",
"                  ",
"                  ",
"                  "};


const char *metronomeOff_xpm[] = {
"13 13 8 1",
" 	c #242523",
".	c #2D2928",
"+	c #34302F",
"@	c #443D3C",
"#	c #4F4445",
"$	c #685659",
"%	c #826A68",
"&	c #A18282",
"             ",
"             ",
"  .       .  ",
" #%       %# ",
" .&+     +&. ",
"  %$     $%  ",
"  @&     &@  ",
"   &@   @&   ",
"   $%   %$   ",
"   +&. .&+   ",
"    %# #%    ",
"    .   .    ",
"             "};


const char *metronomeOn_xpm[] = {
"13 13 8 1",
" 	c #4D4F4C",
".	c #565150",
"+	c #645C5C",
"@	c #716465",
"#	c #837070",
"$	c #8F7775",
"%	c #977C7B",
"&	c #A68787",
"             ",
"             ",
"  .       .  ",
" @%       %@ ",
" .&.     .&. ",
"  $#     #$  ",
"  +&     &+  ",
"   &+   +&   ",
"   #$   $#   ",
"   .&. .&.   ",
"    %@ @%    ",
"    .   .    ",
"             "};


const char *zoomInOff_xpm[] = {
"12 12 8 1",
" 	c #181917",
".	c #242523",
"+	c #2E2F2D",
"@	c #4D4F4C",
"#	c #5D5F5C",
"$	c #828481",
"%	c #9B9D9A",
"&	c #BCBEBB",
"............",
"............",
"............",
".....$$.....",
".....%%.....",
"...$$&&$$...",
"...&&&&&&...",
".....%%.....",
".....%%.....",
"............",
"............",
"............"};


const char *zoomInOn_xpm[] = {
"12 12 8 1",
" 	c #4D4F4C",
".	c #6B6D6A",
"+	c #7B7D7A",
"@	c #969895",
"#	c #A6A8A5",
"$	c #B4B6B3",
"%	c #C0C2BF",
"&	c #FEFFFC",
"            ",
"            ",
"            ",
"     @@     ",
"     ##     ",
"   @@$$@@   ",
"   %%%%%$   ",
"     ##     ",
"     ##     ",
"            ",
"            ",
"            "};


const char *zoomOutOff_xpm[] = {
"12 12 8 1",
" 	c #181917",
".	c #242523",
"+	c #2E2F2D",
"@	c #4D4F4C",
"#	c #5D5F5C",
"$	c #828481",
"%	c #9B9D9A",
"&	c #BCBEBB",
"............",
"............",
"............",
"............",
"............",
"...$$$$$$...",
"...&&&&&&...",
"............",
"............",
"............",
"............",
"............"};


const char *zoomOutOn_xpm[] = {
"12 12 8 1",
" 	c #4D4F4C",
".	c #6B6D6A",
"+	c #7B7D7A",
"@	c #969895",
"#	c #A6A8A5",
"$	c #B4B6B3",
"%	c #C0C2BF",
"&	c #FEFFFC",
"            ",
"            ",
"            ",
"            ",
"            ",
"   @@@@@@   ",
"   %%%%%%   ",
"            ",
"            ",
"            ",
"            ",
"            "};



const char *scrollRightOff_xpm[] = {
"12 12 8 1",
" 	c #181917",
".	c #242523",
"+	c #2E2F2D",
"@	c #4D4F4C",
"#	c #5D5F5C",
"$	c #828481",
"%	c #9B9D9A",
"&	c #BCBEBB",
"............",
"............",
"...+........",
"...&$@......",
"...$&&%@....",
"....+#%&%...",
"....+#%&%...",
"...$&&%#....",
"...&$@......",
"...+........",
"............",
"............"};


const char *scrollLeftOff_xpm[] = {
"12 12 8 1",
" 	c #181917",
".	c #242523",
"+	c #2E2F2D",
"@	c #4D4F4C",
"#	c #5D5F5C",
"$	c #828481",
"%	c #9B9D9A",
"&	c #BCBEBB",
"............",
"............",
"........+...",
"......@$&...",
"....@%&&$...",
"...%&%#+....",
"...%&%#+....",
"....#%&&$...",
"......@$&...",
"........+...",
"............",
"............"};


const char *scrollLeftOn_xpm[] = {
"12 12 8 1",
" 	c #4D4F4C",
".	c #6B6D6A",
"+	c #7B7D7A",
"@	c #969895",
"#	c #A6A8A5",
"$	c #B4B6B3",
"%	c #C0C2BF",
"&	c #FEFFFC",
"            ",
"            ",
"            ",
"      .@$   ",
"    +#%%@   ",
"   $%#+     ",
"   %%#+     ",
"    +$%%@   ",
"      .#$   ",
"            ",
"            ",
"            "};


const char *scrollRightOn_xpm[] = {
"12 12 8 1",
" 	c #4D4F4C",
".	c #6B6D6A",
"+	c #7B7D7A",
"@	c #969895",
"#	c #A6A8A5",
"$	c #B4B6B3",
"%	c #C0C2BF",
"&	c #FEFFFC",
"            ",
"            ",
"            ",
"   %@.      ",
"   @%%#.    ",
"     +#%#   ",
"     +#%#   ",
"   @%%#+    ",
"   %@.      ",
"            ",
"            ",
"            "};


const char *soloOn_xpm[] = {
"18 18 8 1",
" 	c #4D4F4C",
".	c #616360",
"+	c #737572",
"@	c #838582",
"#	c #929491",
"$	c #A5A7A4",
"%	c #B1B3B0",
"&	c #C6C8C5",
"                  ",
"                  ",
"                  ",
"                  ",
"       .@+.       ",
"      #&&&&#      ",
"     .&$  %&.     ",
"      &%+ ..      ",
"      #&&&$.      ",
"       .@$&&.     ",
"     .#.  @&@     ",
"     .&$. #&+     ",
"      #&&&&$      ",
"       .+@+       ",
"                  ",
"                  ",
"                  ",
"                  "};


const char *soloOff_xpm[] = {
"18 18 8 1",
" 	c #242523",
".	c #3D3F3D",
"+	c #525451",
"@	c #666865",
"#	c #80827F",
"$	c #979996",
"%	c #A7A9A6",
"&	c #C6C8C5",
"                  ",
"                  ",
"                  ",
"                  ",
"       .@@.       ",
"      #&&&&#      ",
"     .&$  %&.     ",
"      &%+ ..      ",
"      #&&&$+      ",
"       .@%&&.     ",
"     +#.  @&@     ",
"     .&$..#&+     ",
"      #&&&&$      ",
"       .@@+       ",
"                  ",
"                  ",
"                  ",
"                  "};


#ifdef WITH_VST


const char *fxOff_xpm[] = {
"18 18 8 1",
" 	c #242523",
".	c #40423F",
"+	c #4D4E4C",
"@	c #686A67",
"#	c #7B7D7A",
"$	c #919390",
"%	c #AEB0AD",
"&	c #C1C3C0",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"   ..... .   .    ",
"   $&&&$ $% @&.   ",
"   $$    .&#&@    ",
"   $%##.  @&$     ",
"   $%##.  #&%     ",
"   $$    .&@&#    ",
"   $$    %$ @&.   ",
"   ..    +   +.   ",
"                  ",
"                  ",
"                  ",
"                  ",
"                  "};


const char *fxOn_xpm[] = {
"18 18 8 1",
" 	c #4D4F4C",
".	c #565855",
"+	c #636562",
"@	c #80827F",
"#	c #8E908D",
"$	c #9FA19E",
"%	c #B1B3B0",
"&	c #C1C3C0",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"   .++++ +.  +.   ",
"   $&&&$ $% @&.   ",
"   $$    .&#&@    ",
"   $%##+  @&$     ",
"   $%##+  #&%     ",
"   $$    +&@&#    ",
"   $$    %$ @&+   ",
"   ++   .+.  ++   ",
"                  ",
"                  ",
"                  ",
"                  ",
"                  "};


const char *fxShiftUpOff_xpm[] = {
"18 18 7 1",
" 	c #242523",
".	c #4D4F4C",
"+	c #A3A5A2",
"@	c #868885",
"#	c #C1C3C0",
"$	c #313330",
"%	c #626361",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"       .+@        ",
"       @+#.       ",
"      $#%+@       ",
"      %# %#$      ",
"      +@ $#%      ",
"     $#.  @+      ",
"     $.   $.      ",
"                  ",
"                  ",
"                  ",
"                  ",
"                  "};


const char *fxShiftUpOn_xpm[] = {
"18 18 5 1",
" 	c #4D4F4C",
".	c #70726F",
"+	c #A5A7A4",
"@	c #C1C3BF",
"#	c #8E908D",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"       .++        ",
"       +@@.       ",
"       @.+#       ",
"      .@ .@       ",
"      +#  @.      ",
"     .@.  #+      ",
"      .    .      ",
"                  ",
"                  ",
"                  ",
"                  ",
"                  "};


const char *fxShiftDownOff_xpm[] = {
"18 18 7 1",
" 	c #242523",
".	c #4D4F4C",
"+	c #A3A5A2",
"@	c #313330",
"#	c #626361",
"$	c #868885",
"%	c #C1C3C0",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"     .+@  #$      ",
"     @%#  +$      ",
"      $+ .%@      ",
"      .%@$+       ",
"       +$%#       ",
"       #%%@       ",
"       @..        ",
"                  ",
"                  ",
"                  ",
"                  ",
"                  "};


const char *fxShiftDownOn_xpm[] = {
"18 18 5 1",
" 	c #4D4F4C",
".	c #70726F",
"+	c #A5A7A4",
"@	c #C1C3BF",
"#	c #8E908D",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"                  ",
"     .+   .+      ",
"      @.  +#      ",
"      #+ .@.      ",
"      .@.#+       ",
"       +#@.       ",
"       #@@        ",
"        ..        ",
"                  ",
"                  ",
"                  ",
"                  ",
"                  "};


const char *vstLogo_xpm[] = {
"65 38 8 1",
" 	c #161715",
".	c #2B2D2A",
"+	c #474846",
"@	c #6A6C69",
"#	c #8C8E8B",
"$	c #A8AAA7",
"%	c #C7C9C6",
"&	c #EEF0ED",
" @#############################################################+ ",
"@#.............................................................$+",
"#.                                                             .#",
"#.                                                             .#",
"#.                             ......      ..                  .#",
"#.                         .@$$$####$%$#@.+&$                  .#",
"#.                       .#$$#+.     +#$%%%%$                  .#",
"#.                      .$$#$          .#$$%$                  .#",
"#. .............    ....$$$$$          ++$$%$+@@@@@@@@@@@@@@@  .#",
"#. ##$$$$$$%%%%@    %%&&&&%%$@         %&@#$$@@$%%&&&%%%&&&&&  .#",
"#.   +$$$$$%@         .&%####%$@       $&$.$#  #$%%%&    @&%&  .#",
"#.    +$$$$$%         +&$###$%%&&$@.   $&.     #$%%%&.    .%&  .#",
"#.     @$$$$%$        %##$##$%&&&&&&%#.%#      #$$%%&.     @&  .#",
"#.      $$$$$%+      #&  #$$%%&&&&&&&%%$$@     #$$%%&.      +  .#",
"#.      .$$$$$%     +&+   .#%&&&&&&&&%$$#$$#   #$$%%&.         .#",
"#.       @$$$$%$    %$       @%&&&&&&%$$###$$  #$$%%&.         .#",
"#.        #$$$%%@  #&  .        +$&&&%$####$%$ #$$%%&.         .#",
"#.         $$$%%% .&@ +%#          .@$$$###$$% #$$$%&.         .#",
"#.         +%$%%%$$%  +$$+             #$#$$$% @$$$%&.         .#",
"#.          #%%%%%&.  +%$$              ##$$%$ @$$$%%.         .#",
"#.           $$%%%@   +%$$$.            #$$$%. @$$$$%.         .#",
"#.           +%%%$    +%$$#$@          +$$%$   @#$$$%+         .#",
"#.            @%%.    +%%%$$$$#@++.++@#$$$@ @@##$$$%%%$$@      .#",
"#.             #@     +&#  .@@###$$$###@.   @+++@@@@###$@      .#",
"#.                                                             .#",
"#.                                                             .#",
"#.                                                             .#",
"#.                                                             .#",
"#.                                                             .#",
"#.                   .@$$$$$$$$  .$%%%%%%#                     .#",
"#.                  .......      .@@@@@@@@@.                   .#",
"#.                 ........   @@@+@@@@@@@@@@+                  .#",
"@#                .........  .####@@@@@@@@@@@+                 #@",
" @$$$$$$$$$$$$$$$..........  .@@@@@@@@@@@@@@@@@$$$$$$$$$$$$$$$$@ ",
"                  .........  .@@@@@@@@@@@@@@@.                   ",
"                   ........       @@@@@@@@@@.                    ",
"                    ...........  .@@@@@@@@@                      ",
"                     ..........  .@@@@@@@@                       "};


const char *fxRemoveOff_xpm[] = {
"18 18 9 1",
" 	c None",
".	c #242623",
"+	c #2F312E",
"@	c #393A38",
"#	c #484A47",
"$	c #5D5F5C",
"%	c #8E908D",
"&	c #9B9D9A",
"*	c #BDBFBC",
"..................",
"..................",
"..................",
"..................",
"..................",
".....+#@..@#+.....",
"......&*++*&......",
"......@*%%*@......",
".......$**$.......",
".......#**#.......",
"......+*&&*+......",
"......%*@@*%......",
"......@@..@@......",
"..................",
"..................",
"..................",
"..................",
".................."};


const char *fxRemoveOn_xpm[] = {
"18 18 9 1",
" 	c None",
".	c #4D4F4C",
"+	c #575956",
"@	c #5C5D5B",
"#	c #666865",
"$	c #787977",
"%	c #9C9E9B",
"&	c #A6A8A5",
"*	c #BFC1BE",
"..................",
"..................",
"..................",
"..................",
"..................",
"......#@..@#......",
"......&*++*&......",
"......@*%%*@......",
".......$**$.......",
".......#**#.......",
"......+*&&*+......",
"......%*@+*%......",
"......@+..+@......",
"..................",
"..................",
"..................",
"..................",
".................."};
#endif 
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "mixer.h"
#include "patch.h"
#include "gui_utils.h"
#include "graphics.h"
#include "gd_warnings.h"
#include "ge_window.h"
#include "gd_mainWindow.h"
#include "recorder.h"
#include "wave.h"
#include "channel.h"


extern Mixer 	       G_Mixer;
extern unsigned      G_beats;
extern bool 		     G_audio_status;
extern Patch         G_patch;
extern Conf          G_conf;
extern uint32_t      G_time;
extern gdMainWindow *mainWin;

static int blinker = 0;


void gu_refresh() {
	Fl::lock();

	

	mainWin->outMeter->mixerPeak = G_Mixer.peakOut;
	mainWin->inMeter->mixerPeak  = G_Mixer.peakIn;
	mainWin->outMeter->redraw();
	mainWin->inMeter->redraw();
	mainWin->beatMeter->redraw();

	

	__gu_refreshColumn(mainWin->keyboard->gChannelsL);
	__gu_refreshColumn(mainWin->keyboard->gChannelsR);

	blinker++;
	if (blinker > 12)
		blinker = 0;

	

	Fl::unlock();
	Fl::awake();
}





void __gu_blinkChannel(gChannel *gch) {
	if (blinker > 6) {
		gch->sampleButton->bgColor0 = COLOR_BG_2;
		gch->sampleButton->bdColor  = COLOR_BD_1;
		gch->sampleButton->txtColor = COLOR_TEXT_1;
	}
	else {
		gch->sampleButton->bgColor0 = COLOR_BG_0;
		gch->sampleButton->bdColor  = COLOR_BD_0;
		gch->sampleButton->txtColor = COLOR_TEXT_0;
	}
}





void __gu_refreshColumn(Fl_Group *col) {
	for (int i=0; i<col->children(); i++)	{
		gChannel *gch = (gChannel *) col->child(i);
		gch->refresh();
	}
}





void gu_trim_label(const char *str, unsigned n, Fl_Widget *w) {

	

	

	if (strlen(str) < n)
		w->copy_label(str);
	else {
		char out[FILENAME_MAX];
		strncpy(out, str, n);
		out[n] = '\0';
		strcat(out, "...");
		w->copy_label(out);
	}
}





void gu_update_controls() {

	for (unsigned i=0; i<G_Mixer.channels.size; i++)
		G_Mixer.channels.at(i)->guiChannel->update();

	mainWin->outVol->value(G_Mixer.outVol);
	mainWin->inVol->value(G_Mixer.inVol);

	

	mainWin->beat_stop->value(G_Mixer.running);

	

	int size = G_Mixer.bpm < 100.0f ? 5 : 6;
	char buf_bpm[6];
	snprintf(buf_bpm, size, "%f", G_Mixer.bpm);
	mainWin->bpm->copy_label(buf_bpm);

	char buf_batt[8];
	sprintf(buf_batt, "%d/%d", G_Mixer.beats, G_Mixer.bars);
	mainWin->beats->copy_label(buf_batt);

	if 			(G_Mixer.quantize == 6)		mainWin->quantize->value(5);
	else if (G_Mixer.quantize == 8)		mainWin->quantize->value(6);
	else		mainWin->quantize->value(G_Mixer.quantize);

	mainWin->metronome->value(0);
	mainWin->metronome->redraw();
}





void gu_update_win_label(const char *c) {
	std::string out = VERSIONE_STR;
	out += " - ";
	out += c;
	mainWin->copy_label(out.c_str());
}





void gu_setFavicon(Fl_Window *w) {
#if defined(__linux__)
	fl_open_display();
	Pixmap p, mask;
	XpmCreatePixmapFromData(
		fl_display,
		DefaultRootWindow(fl_display),
		(char **)giada_icon,
		&p,
		&mask,
		NULL);
	w->icon((char *)p);
#elif defined(_WIN32)
	w->icon((char *)LoadIcon(fl_display, MAKEINTRESOURCE(IDI_ICON1)));
#endif
}





void gu_openSubWindow(gWindow *parent, gWindow *child, int id) {
	if (parent->hasWindow(id)) {
		printf("[GU] parent has subwindow with id=%d, deleting\n", id);
		parent->delSubWindow(id);
	}
	child->setId(id);
	parent->addSubWindow(child);
}





void gu_refreshActionEditor() {

	

	gdActionEditor *aeditor = (gdActionEditor*) mainWin->getChild(WID_ACTION_EDITOR);
	if (aeditor) {
		Channel *chan = aeditor->chan;
		mainWin->delSubWindow(WID_ACTION_EDITOR);
		gu_openSubWindow(mainWin, new gdActionEditor(chan), WID_ACTION_EDITOR);
	}
}





gWindow *gu_getSubwindow(gWindow *parent, int id) {
	if (parent->hasWindow(id))
		return parent->getChild(id);
	else
		return NULL;
}





void gu_closeAllSubwindows() {

	

	mainWin->delSubWindow(WID_ACTION_EDITOR);
	mainWin->delSubWindow(WID_SAMPLE_EDITOR);
	mainWin->delSubWindow(WID_FX_LIST);
	mainWin->delSubWindow(WID_FX);
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "init.h"
#include "mixer.h"
#include "wave.h"
#include "const.h"
#include "utils.h"
#include "mixerHandler.h"
#include "patch.h"
#include "conf.h"
#include "pluginHost.h"
#include "recorder.h"
#include "gd_mainWindow.h"
#include "gui_utils.h"
#include "gd_warnings.h"
#include "kernelMidi.h"


extern Mixer 			 G_Mixer;
extern bool		 		 G_audio_status;

#ifdef WITH_VST
extern PluginHost	 G_PluginHost;
#endif



Patch 	        G_Patch;
Conf		        G_Conf;
gdMainWindow   *mainWin;


void init_prepareParser() {

	puts("Giada "VERSIONE" setup...");
	puts("[INIT] loading configuration file...");
	G_Conf.read();
	G_Patch.setDefault();
}





void init_prepareKernelAudio() {
	kernelAudio::openDevice(
		G_Conf.soundSystem,
		G_Conf.soundDeviceOut,
		G_Conf.soundDeviceIn,
		G_Conf.channelsOut,
		G_Conf.channelsIn,
		G_Conf.samplerate,
		G_Conf.buffersize);
	G_Mixer.init();
	recorder::init();
}





void init_prepareKernelMIDI() {
	kernelMidi::setApi(G_Conf.midiSystem);
	kernelMidi::openOutDevice(G_Conf.midiPortOut);
	kernelMidi::openInDevice(G_Conf.midiPortIn);
}





void init_startGUI(int argc, char **argv) {

	int x = (Fl::w() / 2) - (GUI_WIDTH / 2);
	int y = (Fl::h() / 2) - (GUI_HEIGHT / 2);

	char win_label[32];
	sprintf(win_label, "%s - %s",
					VERSIONE_STR,
					!strcmp(G_Patch.name, "") ? "(default patch)" : G_Patch.name);

	mainWin = new gdMainWindow(x, y, GUI_WIDTH, GUI_HEIGHT, win_label, argc, argv);

	

	if (G_audio_status)
		gu_update_controls();

	if (!G_audio_status)
		gdAlert(
			"Your soundcard isn't configured correctly.\n"
			"Check the configuration and restart Giada."
		);
}




void init_startKernelAudio() {
	if (G_audio_status)
		kernelAudio::startStream();

#ifdef WITH_VST
	G_PluginHost.allocBuffers();
#endif
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include <vector>
#include "rtaudio/RtAudio.h"
#include "kernelAudio.h"
#include "glue.h"
#include "conf.h"


extern Mixer G_Mixer;
extern Conf  G_Conf;
extern bool	 G_audio_status;


namespace kernelAudio {

RtAudio  *system       = NULL;
unsigned  numDevs      = 0;
bool 		  inputEnabled = 0;
unsigned  realBufsize  = 0;
int       api          = 0;

int openDevice(
	int _api,
	int outDev,
	int inDev,
	int outChan,
	int inChan,
	int samplerate,
	int buffersize)
{
	api = _api;
	printf("[KA] using system 0x%x\n", api);
#if defined(__linux__)
	if (api == SYS_API_JACK && hasAPI(RtAudio::UNIX_JACK))
		system = new RtAudio(RtAudio::UNIX_JACK);
	else
	if (api == SYS_API_ALSA && hasAPI(RtAudio::LINUX_ALSA))
		system = new RtAudio(RtAudio::LINUX_ALSA);
	else
	if (api == SYS_API_PULSE && hasAPI(RtAudio::LINUX_PULSE))
		system = new RtAudio(RtAudio::LINUX_PULSE);
#elif defined(_WIN32)
	if (api == SYS_API_DS && hasAPI(RtAudio::WINDOWS_DS))
		system = new RtAudio(RtAudio::WINDOWS_DS);
	else
	if (api == SYS_API_ASIO && hasAPI(RtAudio::WINDOWS_ASIO))
		system = new RtAudio(RtAudio::WINDOWS_ASIO);
#elif defined(__APPLE__)
	if (api == SYS_API_CORE && hasAPI(RtAudio::MACOSX_CORE))
		system = new RtAudio(RtAudio::MACOSX_CORE);
#endif
	else {
		G_audio_status = false;
		return 0;
	}



	

	printf("[KA] Opening devices %d (out), %d (in), f=%d...\n", outDev, inDev, samplerate);

	numDevs = system->getDeviceCount();

	if (numDevs < 1) {
		printf("[KA] no devices found with this API\n");
		closeDevice();
		G_audio_status = false;
		return 0;
	}
	else {
		printf("[KA] %d device(s) found\n", numDevs);
		for (unsigned i=0; i<numDevs; i++)
			printf("  %d) %s\n", i, getDeviceName(i));
	}


	RtAudio::StreamParameters outParams;
	RtAudio::StreamParameters inParams;

	if (outDev == DEFAULT_SOUNDDEV_OUT)
		outParams.deviceId = getDefaultOut();
	else
		outParams.deviceId = outDev;
	outParams.nChannels = 2;
	outParams.firstChannel = outChan*2; 

	

	if (inDev != -1) {
		inParams.deviceId     = inDev;
		inParams.nChannels    = 2;
		inParams.firstChannel = inChan*2;   
		inputEnabled = true;
	}
	else
		inputEnabled = false;


  RtAudio::StreamOptions options;
  options.streamName = "Giada";
  options.numberOfBuffers = 4;

	realBufsize = buffersize;

#if defined(__linux__) || defined(__APPLE__)
	if (api == SYS_API_JACK) {
		samplerate = getFreq(outDev, 0);
		printf("[KA] JACK in use, freq = %d\n", samplerate);
		G_Conf.samplerate = samplerate;
	}
#endif

	try {
		if (inDev != -1) {
			system->openStream(
				&outParams, 					
				&inParams, 			  		
				RTAUDIO_FLOAT32,			
				samplerate, 					
				&realBufsize, 				
				&G_Mixer.masterPlay,  
				NULL,									
				&options);
		}
		else {
			system->openStream(
				&outParams, 					
				NULL, 	     		  		
				RTAUDIO_FLOAT32,			
				samplerate, 					
				&realBufsize, 				
				&G_Mixer.masterPlay,  
				NULL,									
				&options);
		}
		G_audio_status = true;

#if defined(__linux__)
		if (api == SYS_API_JACK)
			jackSetSyncCb();
#endif

		return 1;
	}
	catch (RtError &e) {
		printf("[KA] system init error: %s\n", e.getMessage().c_str());
		closeDevice();
		G_audio_status = false;
		return 0;
	}
}





int startStream() {
	try {
		system->startStream();
		printf("[KA] latency = %lu\n", system->getStreamLatency());
		return 1;
	}
	catch (RtError &e) {
		printf("[KA] Start stream error\n");
		return 0;
	}
}





int stopStream() {
	try {
		system->stopStream();
		return 1;
	}
	catch (RtError &e) {
		printf("[KA] Stop stream error\n");
		return 0;
	}
}





const char *getDeviceName(unsigned dev) {
	try {
		return ((RtAudio::DeviceInfo) system->getDeviceInfo(dev)).name.c_str();
	}
	catch (RtError &e) {
		printf("[KA] invalid device ID = %d\n", dev);
		return NULL;
	}
}





int closeDevice() {
	if (system->isStreamOpen()) {
#if defined(__linux__) || defined(__APPLE__)
		system->abortStream(); 
#elif defined(_WIN32)
		system->stopStream();	 
#endif
		system->closeStream();
		delete system;
		system = NULL;
	}
	return 1;
}





unsigned getMaxInChans(int dev) {

	if (dev == -1) return 0;

	try {
		return ((RtAudio::DeviceInfo) system->getDeviceInfo(dev)).inputChannels;
	}
	catch (RtError &e) {
		puts("[KA] Unable to get input channels");
		return 0;
	}
}





unsigned getMaxOutChans(unsigned dev) {
	try {
		return ((RtAudio::DeviceInfo) system->getDeviceInfo(dev)).outputChannels;
	}
	catch (RtError &e) {
		puts("[KA] Unable to get output channels");
		return 0;
	}
}





bool isProbed(unsigned dev) {
	try {
		return ((RtAudio::DeviceInfo) system->getDeviceInfo(dev)).probed;
	}
	catch (RtError &e) {
		return 0;
	}
}





unsigned getDuplexChans(unsigned dev) {
	try {
		return ((RtAudio::DeviceInfo) system->getDeviceInfo(dev)).duplexChannels;
	}
	catch (RtError &e) {
		return 0;
	}
}





bool isDefaultIn(unsigned dev) {
	try {
		return ((RtAudio::DeviceInfo) system->getDeviceInfo(dev)).isDefaultInput;
	}
	catch (RtError &e) {
		return 0;
	}
}





bool isDefaultOut(unsigned dev) {
	try {
		return ((RtAudio::DeviceInfo) system->getDeviceInfo(dev)).isDefaultOutput;
	}
	catch (RtError &e) {
		return 0;
	}
}





int getTotalFreqs(unsigned dev) {
	try {
		return ((RtAudio::DeviceInfo) system->getDeviceInfo(dev)).sampleRates.size();
	}
	catch (RtError &e) {
		return 0;
	}
}





int	getFreq(unsigned dev, int i) {
	try {
		return ((RtAudio::DeviceInfo) system->getDeviceInfo(dev)).sampleRates.at(i);
	}
	catch (RtError &e) {
		return 0;
	}
}





int getDefaultIn() {
	return system->getDefaultInputDevice();
}

int getDefaultOut() {
	return system->getDefaultOutputDevice();
}





int	getDeviceByName(const char *name) {
	for (unsigned i=0; i<numDevs; i++)
		if (strcmp(name, getDeviceName(i))==0)
			return i;
	return -1;
}





bool hasAPI(int API) {
	std::vector<RtAudio::Api> APIs;
	RtAudio::getCompiledApi(APIs);
	for (unsigned i=0; i<APIs.size(); i++)
		if (APIs.at(i) == API)
			return true;
	return false;
}





std::string getRtAudioVersion() {
	return RtAudio::getVersion();
}





#ifdef __linux__
#include <jack/jack.h>
#include <jack/intclient.h>
#include <jack/transport.h>

jack_client_t *jackGetHandle() {
	return (jack_client_t*) system->rtapi_->__HACK__getJackClient();
}

void jackStart() {
	if (api == SYS_API_JACK) {
		jack_client_t *client = jackGetHandle();
		jack_transport_start(client);
	}
}


void jackStop() {
	if (api == SYS_API_JACK) {
		jack_client_t *client = jackGetHandle();
		jack_transport_stop(client);
	}
}


void jackSetSyncCb() {
	jack_client_t *client = jackGetHandle();
	jack_set_sync_callback(client, jackSyncCb, NULL);
	
}


int jackSyncCb(jack_transport_state_t state, jack_position_t *pos,
		void *arg)
{
	switch (state) {
		case JackTransportStopped:
			printf("[KA] Jack transport stopped, frame=%d\n", pos->frame);
			glue_stopSeq(false);  
			if (pos->frame == 0)
				glue_rewindSeq();
			break;

		case JackTransportRolling:
			printf("[KA] Jack transport rolling\n");
			break;

		case JackTransportStarting:
			printf("[KA] Jack transport starting, frame=%d\n", pos->frame);
			glue_startSeq(false);  
			if (pos->frame == 0)
				glue_rewindSeq();
			break;

		default:
			printf("[KA] Jack transport [unknown]\n");
	}
	return 1;
}

#endif

}


/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include <stdio.h>
#include "kernelMidi.h"
#include "glue.h"
#include "mixer.h"
#include "channel.h"
#include "sampleChannel.h"
#include "pluginHost.h"
#include "conf.h"


extern bool  G_midiStatus;
extern Conf  G_Conf;
extern Mixer G_Mixer;

#ifdef WITH_VST
extern PluginHost G_PluginHost;
#endif


namespace kernelMidi {


int        api         = 0;      
RtMidiOut *midiOut     = NULL;
RtMidiIn  *midiIn      = NULL;
unsigned   numOutPorts = 0;
unsigned   numInPorts  = 0;

cb_midiLearn *cb_learn = NULL;
void         *cb_data  = NULL;





void startMidiLearn(cb_midiLearn *cb, void *data) {
	cb_learn = cb;
	cb_data  = data;
}





void stopMidiLearn() {
	cb_learn = NULL;
	cb_data  = NULL;
}





void setApi(int _api) {
	api = api;
	printf("[KM] using system 0x%x\n", api);
}





int openOutDevice(int port) {

	try {
		midiOut = new RtMidiOut((RtMidi::Api) api, "Giada MIDI Output");
		G_midiStatus = true;
  }
  catch (RtError &error) {
    printf("[KM] MIDI out device error: %s\n", error.getMessage().c_str());
    G_midiStatus = false;
    return 0;
  }

	

	numOutPorts = midiOut->getPortCount();
  printf("[KM] %d output MIDI ports found\n", numOutPorts);
  for (unsigned i=0; i<numOutPorts; i++)
		printf("  %d) %s\n", i, getOutPortName(i));

	

	if (port != -1 && numOutPorts > 0) {
		try {
			midiOut->openPort(port, getOutPortName(port));
			printf("[KM] MIDI out port %d open\n", port);
			return 1;
		}
		catch (RtError &error) {
			printf("[KM] unable to open MIDI out port %d: %s\n", port, error.getMessage().c_str());
			G_midiStatus = false;
			return 0;
		}
	}
	else
		return 2;
}





int openInDevice(int port) {

	try {
		midiIn = new RtMidiIn((RtMidi::Api) api, "Giada MIDI input");
		G_midiStatus = true;
  }
  catch (RtError &error) {
    printf("[KM] MIDI in device error: %s\n", error.getMessage().c_str());
    G_midiStatus = false;
    return 0;
  }

	

	numInPorts = midiIn->getPortCount();
  printf("[KM] %d input MIDI ports found\n", numInPorts);
  for (unsigned i=0; i<numInPorts; i++)
		printf("  %d) %s\n", i, getInPortName(i));

	

	if (port != -1 && numInPorts > 0) {
		try {
			midiIn->openPort(port, getInPortName(port));
			midiIn->ignoreTypes(true, false, true); 
			printf("[KM] MIDI in port %d open\n", port);
			midiIn->setCallback(&callback);
			return 1;
		}
		catch (RtError &error) {
			printf("[KM] unable to open MIDI in port %d: %s\n", port, error.getMessage().c_str());
			G_midiStatus = false;
			return 0;
		}
	}
	else
		return 2;
}





bool hasAPI(int API) {
	std::vector<RtMidi::Api> APIs;
	RtMidi::getCompiledApi(APIs);
	for (unsigned i=0; i<APIs.size(); i++)
		if (APIs.at(i) == API)
			return true;
	return false;
}





const char *getOutPortName(unsigned p) {
	try { return midiOut->getPortName(p).c_str(); }
	catch (RtError &error) { return NULL; }
}

const char *getInPortName(unsigned p) {
	try { return midiIn->getPortName(p).c_str(); }
	catch (RtError &error) { return NULL; }
}





void send(uint32_t data) {

	if (!G_midiStatus)
		return;

	std::vector<unsigned char> msg(1, 0x00);
	msg[0] = getB1(data);
	msg[1] = getB2(data);
	msg[2] = getB3(data);

	midiOut->sendMessage(&msg);
	
}





void send(int b1, int b2, int b3) {

	if (!G_midiStatus)
		return;

	std::vector<unsigned char> msg(1, b1);

	if (b2 != -1)
		msg.push_back(b2);
	if (b3 != -1)
		msg.push_back(b3);

	midiOut->sendMessage(&msg);
	
}





void callback(double t, std::vector<unsigned char> *msg, void *data) {

	

	if (msg->size() < 3) {
		printf("[KM] MIDI received - unkown signal - size=%d, value=0x", (int) msg->size());
		for (unsigned i=0; i<msg->size(); i++)
			printf("%X", (int) msg->at(i));
		printf("\n");
		return;
	}

	

	uint32_t input = getIValue(msg->at(0), msg->at(1), msg->at(2));
	uint32_t chan  = input & 0x0F000000;
	uint32_t value = input & 0x0000FF00;
	uint32_t pure  = input & 0xFFFF0000;   

	printf("[KM] MIDI received - 0x%X (chan %d)", input, chan >> 24);

	

	if (cb_learn)	{
		printf("\n");
		cb_learn(pure, cb_data);
	}
	else {

		

		if      (pure == G_Conf.midiInRewind) {
			printf(" >>> rewind (global) (pure=0x%X)", pure);
			glue_rewindSeq();
		}
		else if (pure == G_Conf.midiInStartStop) {
			printf(" >>> startStop (global) (pure=0x%X)", pure);
			glue_startStopSeq();
		}
		else if (pure == G_Conf.midiInActionRec) {
			printf(" >>> actionRec (global) (pure=0x%X)", pure);
			glue_startStopActionRec();
		}
		else if (pure == G_Conf.midiInInputRec) {
			printf(" >>> inputRec (global) (pure=0x%X)", pure);
			glue_startStopInputRec(false, false);   
		}
		else if (pure == G_Conf.midiInMetronome) {
			printf(" >>> metronome (global) (pure=0x%X)", pure);
			glue_startStopMetronome(false);
		}
		else if (pure == G_Conf.midiInVolumeIn) {
			float vf = (value >> 8)/127.0f;
			printf(" >>> input volume (global) (pure=0x%X, value=%d, float=%f)", pure, value >> 8, vf);
			glue_setInVol(vf, false);
		}
		else if (pure == G_Conf.midiInVolumeOut) {
			float vf = (value >> 8)/127.0f;
			printf(" >>> output volume (global) (pure=0x%X, value=%d, float=%f)", pure, value >> 8, vf);
			glue_setOutVol(vf, false);
		}
		else if (pure == G_Conf.midiInBeatDouble) {
			printf(" >>> sequencer x2 (global) (pure=0x%X)", pure);
			glue_beatsMultiply();
		}
		else if (pure == G_Conf.midiInBeatHalf) {
			printf(" >>> sequencer /2 (global) (pure=0x%X)", pure);
			glue_beatsDivide();
		}

		

		for (unsigned i=0; i<G_Mixer.channels.size; i++) {

			Channel *ch = (Channel*) G_Mixer.channels.at(i);

			if (!ch->midiIn) continue;

			if      (pure == ch->midiInKeyPress) {
				printf(" >>> keyPress, ch=%d (pure=0x%X)", ch->index, pure);
				glue_keyPress(ch, false, false);
			}
			else if (pure == ch->midiInKeyRel) {
				printf(" >>> keyRel ch=%d (pure=0x%X)", ch->index, pure);
				glue_keyRelease(ch, false, false);
			}
			else if (pure == ch->midiInMute) {
				printf(" >>> mute ch=%d (pure=0x%X)", ch->index, pure);
				glue_setMute(ch, false);
			}
			else if (pure == ch->midiInSolo) {
				printf(" >>> solo ch=%d (pure=0x%X)", ch->index, pure);
				ch->solo ? glue_setSoloOn(ch, false) : glue_setSoloOff(ch, false);
			}
			else if (pure == ch->midiInVolume) {
				float vf = (value >> 8)/127.0f;
				printf(" >>> volume ch=%d (pure=0x%X, value=%d, float=%f)", ch->index, pure, value >> 8, vf);
				glue_setChanVol(ch, vf, false);
			}
			else if (pure == ((SampleChannel*)ch)->midiInPitch) {
				float vf = (value >> 8)/(127/4.0f); 
				printf(" >>> pitch ch=%d (pure=0x%X, value=%d, float=%f)", ch->index, pure, value >> 8, vf);
				glue_setPitch(NULL, (SampleChannel*)ch, vf, false);
			}
			else if (pure == ((SampleChannel*)ch)->midiInReadActions) {
				printf(" >>> start/stop read actions ch=%d (pure=0x%X)", ch->index, pure);
				glue_startStopReadingRecs((SampleChannel*)ch, false);
			}
		}
		printf("\n");
	}
}

}  


/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include <pthread.h>
#if defined(__linux__) || defined(__APPLE__)
	#include <unistd.h>
#endif
#include "init.h"
#include "const.h"
#include "mixer.h"
#include "mixerHandler.h"
#include "kernelAudio.h"
#include "recorder.h"
#include "gui_utils.h"
#ifdef WITH_VST
#include "pluginHost.h"
#endif


pthread_t t_video;

void *thread_video(void *arg);

#ifdef WITH_VST
PluginHost 	G_PluginHost;
#endif


Mixer G_Mixer;
bool	G_quit;
bool	G_audio_status;
bool  G_midiStatus;


int main(int argc, char **argv) {

	G_quit = false;

	init_prepareParser();
	init_prepareKernelAudio();
	init_prepareKernelMIDI();
	init_startGUI(argc, argv);
	Fl::lock();
	pthread_create(&t_video, NULL, thread_video, NULL);
	init_startKernelAudio();

	int ret = Fl::run();

	pthread_join(t_video, NULL);
	return ret;
}



void *thread_video(void *arg) {
	if (G_audio_status)
		while (!G_quit)	{
			gu_refresh();
#ifdef _WIN32
			Sleep(GUI_SLEEP);
#else
			usleep(GUI_SLEEP);
#endif
		}
	pthread_exit(NULL);
	return 0;
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "channel.h"
#include "midiChannel.h"
#include "pluginHost.h"
#include "patch.h"
#include "conf.h"
#include "kernelMidi.h"


extern Patch       G_Patch;
extern Mixer       G_Mixer;
extern Conf        G_Conf;
#ifdef WITH_VST
extern PluginHost  G_PluginHost;
#endif


MidiChannel::MidiChannel(int bufferSize, char side)
	: Channel    (CHANNEL_MIDI, STATUS_OFF, side, bufferSize),
	  midiOut    (false),
	  midiOutChan(MIDI_CHANS[0])
{
#ifdef WITH_VST 
	freeVstMidiEvents(true);
#endif
}





MidiChannel::~MidiChannel() {}





#ifdef WITH_VST

void MidiChannel::freeVstMidiEvents(bool init) {
	if (events.numEvents == 0 && !init)
		return;
	memset(events.events, 0, sizeof(VstEvent*) * MAX_VST_EVENTS);
	events.numEvents = 0;
	events.reserved  = 0;
}

#endif





#ifdef WITH_VST

void MidiChannel::addVstMidiEvent(uint32_t msg) {
	addVstMidiEvent(G_PluginHost.createVstMidiEvent(msg));
}

#endif





#ifdef WITH_VST

void MidiChannel::addVstMidiEvent(VstMidiEvent *e) {
	if (events.numEvents < MAX_VST_EVENTS) {
		events.events[events.numEvents] = (VstEvent*) e;
		events.numEvents++;
		
	}
	else
		printf("[MidiChannel] channel %d VstEvents = %d > MAX_VST_EVENTS, nothing to do\n", index, events.numEvents);
}

#endif





void MidiChannel::onBar(int frame) {}





void MidiChannel::stop() {}





void MidiChannel::empty() {}





void MidiChannel::quantize(int index, int localFrame, int globalFrame) {}




#ifdef WITH_VST

VstEvents *MidiChannel::getVstEvents() {
	return (VstEvents *) &events;
}

#endif





void MidiChannel::parseAction(recorder::action *a, int localFrame, int globalFrame) {
	if (a->type == ACTION_MIDI)
		sendMidi(a, localFrame/2);
}





void MidiChannel::onZero(int frame) {
	if (status == STATUS_ENDING)
		status = STATUS_OFF;
	else
	if (status == STATUS_WAIT)
		status = STATUS_PLAY;
}





void MidiChannel::setMute(bool internal) {
	mute = true;  	
	if (midiOut)
		kernelMidi::send(MIDI_ALL_NOTES_OFF);
#ifdef WITH_VST
		addVstMidiEvent(MIDI_ALL_NOTES_OFF);
#endif
}





void MidiChannel::unsetMute(bool internal) {
	mute = false;  	
}





void MidiChannel::process(float *buffer) {
#ifdef WITH_VST
	G_PluginHost.processStack(vChan, PluginHost::CHANNEL, this);
	freeVstMidiEvents();
#endif

	for (int j=0; j<bufferSize; j+=2) {
		buffer[j]   += vChan[j]   * volume; 
		buffer[j+1] += vChan[j+1] * volume; 
	}
}





void MidiChannel::start(int frame, bool doQuantize) {
	switch (status) {
		case STATUS_PLAY:
			status = STATUS_ENDING;
			break;
		case STATUS_ENDING:
		case STATUS_WAIT:
			status = STATUS_OFF;
			break;
		case STATUS_OFF:
			status = STATUS_WAIT;
			break;
	}
}





void MidiChannel::stopBySeq() {
	kill(0);
}





void MidiChannel::kill(int frame) {
	if (status & (STATUS_PLAY | STATUS_ENDING)) {
		if (midiOut)
			kernelMidi::send(MIDI_ALL_NOTES_OFF);
#ifdef WITH_VST
		addVstMidiEvent(MIDI_ALL_NOTES_OFF);
#endif
	}
	status = STATUS_OFF;
}





int MidiChannel::loadByPatch(const char *f, int i) {
	volume      = G_Patch.getVol(i);
	index       = G_Patch.getIndex(i);
	mute        = G_Patch.getMute(i);
	mute_s      = G_Patch.getMute_s(i);
	solo        = G_Patch.getSolo(i);
	panLeft     = G_Patch.getPanLeft(i);
	panRight    = G_Patch.getPanRight(i);

	midiOut     = G_Patch.getMidiValue(i, "Out");
	midiOutChan = G_Patch.getMidiValue(i, "OutChan");

	readPatchMidiIn(i);

	return SAMPLE_LOADED_OK;  
}





void MidiChannel::sendMidi(recorder::action *a, int localFrame) {

	if (status & (STATUS_PLAY | STATUS_ENDING) && !mute) {
		if (midiOut)
			kernelMidi::send(a->iValue | MIDI_CHANS[midiOutChan]);

#ifdef WITH_VST
		a->event->deltaFrames = localFrame;
		addVstMidiEvent(a->event);
#endif
	}
}


void MidiChannel::sendMidi(uint32_t data) {
	if (status & (STATUS_PLAY | STATUS_ENDING) && !mute) {
		if (midiOut)
			kernelMidi::send(data | MIDI_CHANS[midiOutChan]);
#ifdef WITH_VST
		addVstMidiEvent(data);
#endif
	}
}





void MidiChannel::rewind() {
	if (midiOut)
		kernelMidi::send(MIDI_ALL_NOTES_OFF);
#ifdef WITH_VST
		addVstMidiEvent(MIDI_ALL_NOTES_OFF);
#endif
}





void MidiChannel::writePatch(FILE *fp, int i, bool isProject) {
	fprintf(fp, "chanSide%d=%d\n",           i, side);
	fprintf(fp, "chanType%d=%d\n",           i, type);
	fprintf(fp, "chanIndex%d=%d\n",          i, index);
	fprintf(fp, "chanmute%d=%d\n",           i, mute);
	fprintf(fp, "chanMute_s%d=%d\n",         i, mute_s);
	fprintf(fp, "chanSolo%d=%d\n",           i, solo);
	fprintf(fp, "chanvol%d=%f\n",            i, volume);
	fprintf(fp, "chanPanLeft%d=%f\n",        i, panLeft);
	fprintf(fp, "chanPanRight%d=%f\n",       i, panRight);

	

	fprintf(fp, "chanMidiOut%d=%u\n",        i, midiOut);
	fprintf(fp, "chanMidiOutChan%d=%u\n",    i, midiOutChan);

	writePatchMidiIn(fp, i);
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include <math.h>
#include "mixer.h"
#include "init.h"
#include "wave.h"
#include "gui_utils.h"
#include "recorder.h"
#include "pluginHost.h"
#include "patch.h"
#include "conf.h"
#include "mixerHandler.h"
#include "channel.h"
#include "sampleChannel.h"
#include "midiChannel.h"
#include "kernelMidi.h"


extern Mixer 			 G_Mixer;
extern Patch		 	 G_Patch;
extern Conf				 G_Conf;
#ifdef WITH_VST
extern PluginHost  G_PluginHost;
#endif


Mixer::Mixer() 	{}
Mixer::~Mixer() {}


#define TICKSIZE 38


float Mixer::tock[TICKSIZE] = {
	 0.059033,  0.117240,  0.173807,  0.227943,  0.278890,  0.325936,
	 0.368423,  0.405755,  0.437413,  0.462951,  0.482013,  0.494333,
	 0.499738,  0.498153,  0.489598,  0.474195,  0.452159,  0.423798,
	 0.389509,  0.349771,  0.289883,  0.230617,  0.173194,  0.118739,
	 0.068260,  0.022631, -0.017423, -0.051339,	-0.078721, -0.099345,
	-0.113163, -0.120295, -0.121028, -0.115804, -0.105209, -0.089954,
	-0.070862, -0.048844
};


float Mixer::tick[TICKSIZE] = {
	  0.175860,  0.341914,  0.488904,  0.608633,  0.694426,  0.741500,
	  0.747229,  0.711293,	0.635697,  0.524656,  0.384362,  0.222636,
	  0.048496, -0.128348, -0.298035, -0.451105, -0.579021, -0.674653,
	 -0.732667, -0.749830, -0.688924, -0.594091, -0.474481, -0.340160,
	 -0.201360, -0.067752,  0.052194,  0.151746,  0.226280,  0.273493,
	  0.293425,  0.288307,  0.262252,  0.220811,  0.170435,  0.117887,
	  0.069639,  0.031320
};





void Mixer::init() {
	quanto      = 1;
	docross     = false;
	rewindWait  = false;
	running     = false;
	ready       = true;
	waitRec     = 0;
	actualFrame = 0;
	bpm 		    = DEFAULT_BPM;
	bars		    = DEFAULT_BARS;
	beats		    = DEFAULT_BEATS;
	quantize    = DEFAULT_QUANTIZE;
	metronome   = false;

	tickTracker = 0;
	tockTracker = 0;
	tickPlay    = false;
	tockPlay    = false;

	outVol       = DEFAULT_OUT_VOL;
	inVol        = DEFAULT_IN_VOL;
	peakOut      = 0.0f;
	peakIn	     = 0.0f;
	chanInput    = NULL;
	inputTracker = 0;

	actualBeat    = 0;

	midiTCstep    = 0;
	midiTCrate    = (G_Conf.samplerate / G_Conf.midiTCfps) * 2;  
	midiTCframes  = 0;
	midiTCseconds = 0;
	midiTCminutes = 0;
	midiTChours   = 0;

	
	

	vChanInput   = NULL;
	vChanInToOut = (float *) malloc(kernelAudio::realBufsize * 2 * sizeof(float));

	pthread_mutex_init(&mutex_recs, NULL);
	pthread_mutex_init(&mutex_chans, NULL);
	pthread_mutex_init(&mutex_plugins, NULL);

	updateFrameBars();
	rewind();
}





Channel *Mixer::addChannel(char side, int type) {

	Channel *ch;
	int bufferSize = kernelAudio::realBufsize*2;

	if (type == CHANNEL_SAMPLE)
		ch = new SampleChannel(bufferSize, side);
	else
		ch = new MidiChannel(bufferSize, side);

	while (true) {
		int lockStatus = pthread_mutex_trylock(&mutex_chans);
		if (lockStatus == 0) {
			channels.add(ch);
			pthread_mutex_unlock(&mutex_chans);
			break;
		}
	}

	ch->index = getNewIndex();
	printf("[mixer] channel index=%d added, type=%d, total=%d\n", ch->index, ch->type, channels.size);
	return ch;
}





int Mixer::getNewIndex() {

	

	if (channels.size == 1)
		return 0;

	int index = 0;
	for (unsigned i=0; i<channels.size-1; i++) {
		if (channels.at(i)->index > index)
			index = channels.at(i)->index;
		}
	index += 1;
	return index;
}





int Mixer::deleteChannel(Channel *ch) {
	int lockStatus;
	while (true) {
		lockStatus = pthread_mutex_trylock(&mutex_chans);
		if (lockStatus == 0) {
			channels.del(ch);
			delete ch;
			pthread_mutex_unlock(&mutex_chans);
			return 1;
		}
		
		
	}
}





Channel *Mixer::getChannelByIndex(int index) {
	for (unsigned i=0; i<channels.size; i++)
		if (channels.at(i)->index == index)
			return channels.at(i);
	printf("[mixer::getChannelByIndex] channel at index %d not found!\n", index);
	return NULL;
}





void Mixer::sendMIDIsync() {

	if (G_Conf.midiSync == MIDI_SYNC_CLOCK_M) {
		if (actualFrame % (framesPerBeat/24) == 0)
			kernelMidi::send(MIDI_CLOCK, -1, -1);
	}
	else
	if (G_Conf.midiSync == MIDI_SYNC_MTC_M) {

		

		if (actualFrame % midiTCrate == 0) {

			

			if (midiTCframes % 2 == 0) {
				kernelMidi::send(MIDI_MTC_QUARTER, (midiTCframes & 0x0F)  | 0x00, -1);
				kernelMidi::send(MIDI_MTC_QUARTER, (midiTCframes >> 4)    | 0x10, -1);
				kernelMidi::send(MIDI_MTC_QUARTER, (midiTCseconds & 0x0F) | 0x20, -1);
				kernelMidi::send(MIDI_MTC_QUARTER, (midiTCseconds >> 4)   | 0x30, -1);
			}

			

			else {
				kernelMidi::send(MIDI_MTC_QUARTER, (midiTCminutes & 0x0F) | 0x40, -1);
				kernelMidi::send(MIDI_MTC_QUARTER, (midiTCminutes >> 4)   | 0x50, -1);
				kernelMidi::send(MIDI_MTC_QUARTER, (midiTChours & 0x0F)   | 0x60, -1);
				kernelMidi::send(MIDI_MTC_QUARTER, (midiTChours >> 4)     | 0x70, -1);
			}

			midiTCframes++;

			

			if (midiTCframes > G_Conf.midiTCfps) {
				midiTCframes = 0;
				midiTCseconds++;
				if (midiTCseconds >= 60) {
					midiTCminutes++;
					midiTCseconds = 0;
					if (midiTCminutes >= 60) {
						midiTChours++;
						midiTCminutes = 0;
					}
				}
				
			}
		}
	}
}





void Mixer::sendMIDIrewind() {

	midiTCframes  = 0;
	midiTCseconds = 0;
	midiTCminutes = 0;
	midiTChours   = 0;

	

	if (G_Conf.midiSync == MIDI_SYNC_MTC_M) {
		kernelMidi::send(MIDI_SYSEX, 0x7F, 0x00);  
		kernelMidi::send(0x01, 0x01, 0x00);        
		kernelMidi::send(0x00, 0x00, 0x00);        
		kernelMidi::send(MIDI_EOX, -1, -1);        
	}
}




int Mixer::masterPlay(
	void *out_buf, void *in_buf, unsigned n_frames,
	double streamTime, RtAudioStreamStatus status, void *userData) {
	return G_Mixer.__masterPlay(out_buf, in_buf, n_frames);
}





int Mixer::__masterPlay(void *out_buf, void *in_buf, unsigned bufferFrames) {

	if (!ready)
		return 0;

	float *outBuf = ((float *) out_buf);
	float *inBuf  = ((float *) in_buf);
	bufferFrames *= 2;     
	peakOut       = 0.0f;  
	peakIn        = 0.0f;  

	

	memset(outBuf, 0, sizeof(float) * bufferFrames);         
	memset(vChanInToOut, 0, sizeof(float) * bufferFrames);   

	pthread_mutex_lock(&mutex_chans);
	for (unsigned i=0; i<channels.size; i++)
		if (channels.at(i)->type == CHANNEL_SAMPLE)
			((SampleChannel*)channels.at(i))->clear();
	pthread_mutex_unlock(&mutex_chans);

	for (unsigned j=0; j<bufferFrames; j+=2) {

		if (kernelAudio::inputEnabled) {

			

			if (inBuf[j] * inVol > peakIn)
				peakIn = inBuf[j] * inVol;

			

			if (inToOut) {
				vChanInToOut[j]   = inBuf[j]   * inVol;
				vChanInToOut[j+1] = inBuf[j+1] * inVol;
			}
		}

		

		if (running) {

			

			if (chanInput != NULL && kernelAudio::inputEnabled) {

				

				if (waitRec < G_Conf.delayComp)
					waitRec += 2;
				else {
					vChanInput[inputTracker]   += inBuf[j]   * inVol;
					vChanInput[inputTracker+1] += inBuf[j+1] * inVol;
					inputTracker += 2;
					if (inputTracker >= totalFrames)
						inputTracker = 0;
				}
			}

			

			if (quantize > 0 && quanto > 0) {
				if (actualFrame % (quanto) == 0) {   
					if (rewindWait) {
						rewindWait = false;
						rewind();
					}
					pthread_mutex_lock(&mutex_chans);
					for (unsigned k=0; k<channels.size; k++)
						channels.at(k)->quantize(k, j, actualFrame);  
					pthread_mutex_unlock(&mutex_chans);
				}
			}

			

			if (actualFrame % framesPerBar == 0 && actualFrame != 0) {
				if (metronome)
					tickPlay = true;

				pthread_mutex_lock(&mutex_chans);
				for (unsigned k=0; k<channels.size; k++)
					channels.at(k)->onBar(j);
				pthread_mutex_unlock(&mutex_chans);
			}

			

			if (actualFrame == 0) {
				pthread_mutex_lock(&mutex_chans);
				for (unsigned k=0; k<channels.size; k++)
					channels.at(k)->onZero(j);
				pthread_mutex_unlock(&mutex_chans);
			}

			

			pthread_mutex_lock(&mutex_recs);
			for (unsigned y=0; y<recorder::frames.size; y++) {
				if (recorder::frames.at(y) == actualFrame) {
					for (unsigned z=0; z<recorder::global.at(y).size; z++) {
						int index   = recorder::global.at(y).at(z)->chan;
						Channel *ch = getChannelByIndex(index);
						ch->parseAction(recorder::global.at(y).at(z), j, actualFrame);
					}
					break;
				}
			}
			pthread_mutex_unlock(&mutex_recs);

			

			actualFrame += 2;

			

			if (actualFrame > totalFrames) {
				actualFrame = 0;
				actualBeat  = 0;
			}
			else
			if (actualFrame % framesPerBeat == 0 && actualFrame > 0) {
				actualBeat++;

				

				if (metronome && !tickPlay)
					tockPlay = true;
			}

			sendMIDIsync();

		} 

		

		pthread_mutex_lock(&mutex_chans);
		for (unsigned k=0; k<channels.size; k++) {
			if (channels.at(k)->type == CHANNEL_SAMPLE)
				((SampleChannel*)channels.at(k))->sum(j, running);
		}
		pthread_mutex_unlock(&mutex_chans);

		
		

		if (tockPlay) {
			outBuf[j]   += tock[tockTracker];
			outBuf[j+1] += tock[tockTracker];
			tockTracker++;
			if (tockTracker >= TICKSIZE-1) {
				tockPlay    = false;
				tockTracker = 0;
			}
		}
		if (tickPlay) {
			outBuf[j]   += tick[tickTracker];
			outBuf[j+1] += tick[tickTracker];
			tickTracker++;
			if (tickTracker >= TICKSIZE-1) {
				tickPlay    = false;
				tickTracker = 0;
			}
		}
	} 


	

	pthread_mutex_lock(&mutex_chans);
	for (unsigned k=0; k<channels.size; k++)
		channels.at(k)->process(outBuf);
	pthread_mutex_unlock(&mutex_chans);

	

#ifdef WITH_VST
	pthread_mutex_lock(&mutex_plugins);
	G_PluginHost.processStack(outBuf, PluginHost::MASTER_OUT);
	G_PluginHost.processStack(vChanInToOut, PluginHost::MASTER_IN);
	pthread_mutex_unlock(&mutex_plugins);
#endif

	

	for (unsigned j=0; j<bufferFrames; j+=2) {

		

		if (inToOut) {
			outBuf[j]   += vChanInToOut[j];
			outBuf[j+1] += vChanInToOut[j+1];
		}

		outBuf[j]   *= outVol;
		outBuf[j+1] *= outVol;

		

		if (outBuf[j] > peakOut)
			peakOut = outBuf[j];

		if (G_Conf.limitOutput) {
			if (outBuf[j] > 1.0f)
				outBuf[j] = 1.0f;
			else if (outBuf[j] < -1.0f)
				outBuf[j] = -1.0f;

			if (outBuf[j+1] > 1.0f)
				outBuf[j+1] = 1.0f;
			else if (outBuf[j+1] < -1.0f)
				outBuf[j+1] = -1.0f;
		}
	}

	return 0;
}





void Mixer::updateFrameBars() {

	

	float seconds     = (60.0f / bpm) * beats;
	totalFrames       = G_Conf.samplerate * seconds * 2;
	framesPerBar      = totalFrames / bars;
	framesPerBeat     = totalFrames / beats;
	framesInSequencer = framesPerBeat * MAX_BEATS;

	

	if (totalFrames % 2 != 0)
		totalFrames--;
	if (framesPerBar % 2 != 0)
		framesPerBar--;
	if (framesPerBeat % 2 != 0)
		framesPerBeat--;

	updateQuanto();

	

	if (vChanInput != NULL)
		free(vChanInput);
	vChanInput = (float*) malloc(totalFrames * sizeof(float));
	if (!vChanInput)
		printf("[Mixer] vChanInput realloc error!");
}





int Mixer::close() {
	running = false;
	while (channels.size > 0)
		deleteChannel(channels.at(0));
	free(vChanInput);
	free(vChanInToOut);
	return 1;
}





bool Mixer::isSilent() {
	for (unsigned i=0; i<channels.size; i++)
		if (channels.at(i)->status == STATUS_PLAY)
			return false;
	return true;
}





void Mixer::rewind() {

	actualFrame = 0;
	actualBeat  = 0;

	if (running)
		for (unsigned i=0; i<channels.size; i++)
			channels.at(i)->rewind();

	sendMIDIrewind();
}





void Mixer::updateQuanto() {

	

	if (quantize != 0)
		quanto = framesPerBeat / quantize;
	if (quanto % 2 != 0)
		quanto++;
}





bool Mixer::hasLogicalSamples() {
	for (unsigned i=0; i<channels.size; i++)
		if (channels.at(i)->type == CHANNEL_SAMPLE)
			if (((SampleChannel*)channels.at(i))->wave)
				if (((SampleChannel*)channels.at(i))->wave->isLogical)
					return true;
	return false;
}





bool Mixer::hasEditedSamples() {
	for (unsigned i=0; i<channels.size; i++)
		if (channels.at(i)->type == CHANNEL_SAMPLE)
			if (((SampleChannel*)channels.at(i))->wave)
				if (((SampleChannel*)channels.at(i))->wave->isEdited)
					return true;
	return false;
}





bool Mixer::mergeVirtualInput() {
	if (vChanInput == NULL) {
		puts("[Mixer] virtual input channel not alloc'd");
		return false;
	}
	else {
#ifdef WITH_VST
		G_PluginHost.processStackOffline(vChanInput, PluginHost::MASTER_IN, 0, totalFrames);
#endif
		int numFrames = totalFrames*sizeof(float);
		memcpy(chanInput->wave->data, vChanInput, numFrames);
		memset(vChanInput, 0, numFrames); 
		return true;
	}
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "mixerHandler.h"
#include "kernelMidi.h"
#include "mixer.h"
#include "const.h"
#include "utils.h"
#include "init.h"
#include "pluginHost.h"
#include "plugin.h"
#include "waveFx.h"
#include "glue.h"
#include "conf.h"
#include "patch.h"
#include "recorder.h"
#include "channel.h"
#include "sampleChannel.h"
#include "wave.h"


extern Mixer 		  G_Mixer;
extern Patch 		  G_Patch;
extern Conf 		  G_Conf;

#ifdef WITH_VST
extern PluginHost G_PluginHost;
#endif


void mh_stopSequencer() {
	G_Mixer.running = false;
	for (unsigned i=0; i<G_Mixer.channels.size; i++)
		G_Mixer.channels.at(i)->stopBySeq();
}





bool mh_uniqueSolo(Channel *ch) {
	int solos = 0;
	for (unsigned i=0; i<G_Mixer.channels.size; i++) {
		Channel *ch = G_Mixer.channels.at(i);
		if (ch->solo) solos++;
		if (solos > 1) return false;
	}
	return true;
}







void mh_loadPatch(bool isProject, const char *projPath) {

	G_Mixer.init();
	G_Mixer.ready = false;   

	int numChans = G_Patch.getNumChans();
	for (int i=0; i<numChans; i++) {

		Channel *ch = glue_addChannel(G_Patch.getSide(i), G_Patch.getType(i));

		char smpPath[PATH_MAX];

		

		if (isProject && G_Patch.version >= 0.63f)
			sprintf(smpPath, "%s%s%s", gDirname(projPath).c_str(), gGetSlash().c_str(), G_Patch.getSamplePath(i).c_str());
		else
			sprintf(smpPath, "%s", G_Patch.getSamplePath(i).c_str());

		ch->loadByPatch(smpPath, i);
	}

	G_Mixer.outVol     = G_Patch.getOutVol();
	G_Mixer.inVol      = G_Patch.getInVol();
	G_Mixer.bpm        = G_Patch.getBpm();
	G_Mixer.bars       = G_Patch.getBars();
	G_Mixer.beats      = G_Patch.getBeats();
	G_Mixer.quantize   = G_Patch.getQuantize();
	G_Mixer.metronome  = G_Patch.getMetronome();
	G_Patch.lastTakeId = G_Patch.getLastTakeId();
	G_Patch.samplerate = G_Patch.getSamplerate();

	

	G_Mixer.rewind();
	G_Mixer.updateFrameBars();
	G_Mixer.ready = true;
}





void mh_rewindSequencer() {
	if (G_Mixer.quantize > 0 && G_Mixer.running)   
		G_Mixer.rewindWait = true;
	else
		G_Mixer.rewind();
}





SampleChannel *mh_startInputRec() {

	

	SampleChannel *chan = NULL;
	for (unsigned i=0; i<G_Mixer.channels.size; i++) {
		if (G_Mixer.channels.at(i)->type == CHANNEL_SAMPLE)
			if (((SampleChannel*) G_Mixer.channels.at(i))->canInputRec()) {
			chan = (SampleChannel*) G_Mixer.channels.at(i);
			break;
		}
	}

	

	if (chan == NULL)
		return NULL;

	Wave *w = new Wave();
	if (!w->allocEmpty(G_Mixer.totalFrames))
		return NULL;

	

	char name[32];
	sprintf(name, "TAKE-%d", G_Patch.lastTakeId);
	while (!mh_uniqueSamplename(chan, name)) {
		G_Patch.lastTakeId++;
		sprintf(name, "TAKE-%d", G_Patch.lastTakeId);
	}

	chan->allocEmpty(G_Mixer.totalFrames, G_Patch.lastTakeId);
	G_Mixer.chanInput = chan;

	
	

	G_Mixer.inputTracker = G_Mixer.actualFrame;

	printf(
		"[mh] start input recs using chan %d with size %d, frame=%d\n",
		chan->index, G_Mixer.totalFrames, G_Mixer.inputTracker
	);

	return chan;
}





SampleChannel *mh_stopInputRec() {
	printf("[mh] stop input recs\n");
	G_Mixer.mergeVirtualInput();
	SampleChannel *ch = G_Mixer.chanInput;
	G_Mixer.chanInput = NULL;
	G_Mixer.waitRec   = 0;					
	return ch;
}





bool mh_uniqueSamplename(SampleChannel *ch, const char *name) {
	for (unsigned i=0; i<G_Mixer.channels.size; i++) {
		if (ch != G_Mixer.channels.at(i)) {
			if (G_Mixer.channels.at(i)->type == CHANNEL_SAMPLE) {
				SampleChannel *other = (SampleChannel*) G_Mixer.channels.at(i);
				if (other->wave != NULL)
					if (!strcmp(name, other->wave->name.c_str()))
						return false;
			}
		}
	}
	return true;
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include <stdint.h>
#include "patch.h"
#include "init.h"
#include "recorder.h"
#include "utils.h"
#include "conf.h"
#include "pluginHost.h"
#include "wave.h"
#include "mixer.h"
#include "channel.h"


extern Mixer 		  G_Mixer;
extern Conf 		  G_Conf;
#ifdef WITH_VST
extern PluginHost G_PluginHost;
#endif


int Patch::open(const char *file) {
	fp = fopen(file, "r");
	if (fp == NULL)
		return PATCH_UNREADABLE;

	if (getValue("header") != "GIADAPTC")
		return PATCH_INVALID;

	version = atof(getValue("versionf").c_str());
	printf("[PATCH] open patch version %f\n", version);
	if (version == 0.0)
		puts("[PATCH] patch < 0.6.1, backward compatibility mode");

	return PATCH_OPEN_OK;
}





void Patch::setDefault() {
	name[0]    = '\0';
  lastTakeId = 0;
  samplerate = DEFAULT_SAMPLERATE;
}





int Patch::close() {
	return fclose(fp);
}





void Patch::getName() {
	std::string out = getValue("patchname");
	strncpy(name, out.c_str(), MAX_PATCHNAME_LEN);
}





std::string Patch::getSamplePath(int c) {
	char tmp[16];
	sprintf(tmp, "samplepath%d", c);
	return getValue(tmp);
}





float Patch::getPitch(int c) {
	char tmp[16];
	sprintf(tmp, "chanPitch%d", c);
	float out = atof(getValue(tmp).c_str());
	if (out > 2.0f || out < 0.1f)
		return 1.0f;
	return out;
}





int Patch::getNumChans() {
	if (version == 0.0)      
		return 32;
	return atoi(getValue("channels").c_str());
}





char Patch::getSide(int c) {
	if (version == 0.0)      
		return 0;
	char tmp[16];
	sprintf(tmp, "chanSide%d", c);
	return atoi(getValue(tmp).c_str());
}




int Patch::getIndex(int c) {
	if (version == 0.0)      
		return c;

	char tmp[16];
	sprintf(tmp, "chanIndex%d", c);
	return atoi(getValue(tmp).c_str());
}





float Patch::getVol(int c) {
	char tmp[16];
	sprintf(tmp, "chanvol%d", c);
	float out = atof(getValue(tmp).c_str());
	if (out > 1.0f || out < 0.0f)
		return DEFAULT_VOL;
	return out;
}





int Patch::getMode(int c) {
	char tmp[16];
	sprintf(tmp, "chanmode%d", c);
	int out = atoi(getValue(tmp).c_str());
	if (out & (LOOP_ANY | SINGLE_ANY))
		return out;
	return DEFAULT_CHANMODE;
}





int Patch::getMute(int c) {
	char tmp[16];
	sprintf(tmp, "chanmute%d", c);
	return atoi(getValue(tmp).c_str());
}





int Patch::getMute_s(int c) {
	char tmp[16];
	sprintf(tmp, "chanMute_s%d", c);
	return atoi(getValue(tmp).c_str());
}





int Patch::getSolo(int c) {
	char tmp[16];
	sprintf(tmp, "chanSolo%d", c);
	return atoi(getValue(tmp).c_str());
}





int Patch::getType(int c) {
	char tmp[16];
	sprintf(tmp, "chanType%d", c);
	int out = atoi(getValue(tmp).c_str());
	if (out == 0)
		return CHANNEL_SAMPLE;
	return out;
}





int Patch::getBegin(int c) {
	char tmp[16];
	if (version < 0.73f)
		sprintf(tmp, "chanstart%d", c);
	else
		sprintf(tmp, "chanBegin%d", c);
	int out = atoi(getValue(tmp).c_str());
	if (out < 0)
		return 0;
	return out;
}





int Patch::getEnd(int c, unsigned size) {
	char tmp[16];
	sprintf(tmp, "chanend%d", c);

	

	std::string val = getValue(tmp);
	if (val == "")
		return size;

	unsigned out = atoi(val.c_str());
	if (out <= 0 || out > size)
		return size;
	return out;
}





float Patch::getBoost(int c) {
	char tmp[16];
	sprintf(tmp, "chanBoost%d", c);
	float out = atof(getValue(tmp).c_str());
	if (out < 1.0f)
		return DEFAULT_BOOST;
	return out;
}





float Patch::getPanLeft(int c) {
	char tmp[16];
	sprintf(tmp, "chanPanLeft%d", c);
	std::string val = getValue(tmp);
	if (val == "")
		return 1.0f;

	float out = atof(val.c_str());
	if (out < 0.0f || out > 1.0f)
		return 1.0f;
	return out;
}





int Patch::getKey(int c) {
	if (version == 0.0)      
		return 0;
	char tmp[16];
	sprintf(tmp, "chanKey%d", c);
	return atoi(getValue(tmp).c_str());
}





float Patch::getPanRight(int c) {
	char tmp[16];
	sprintf(tmp, "chanPanRight%d", c);
	std::string val = getValue(tmp);
	if (val == "")
		return 1.0f;

	float out = atof(val.c_str());
	if (out < 0.0f || out > 1.0f)
		return 1.0f;
	return out;
}





bool Patch::getRecActive(int c) {
	char tmp[16];
	sprintf(tmp, "chanRecActive%d", c);
	return atoi(getValue(tmp).c_str());
}





float Patch::getOutVol() {
	return atof(getValue("outVol").c_str());
}





float Patch::getInVol() {
	return atof(getValue("inVol").c_str());
}





float Patch::getBpm() {
	float out = atof(getValue("bpm").c_str());
	if (out < 20.0f || out > 999.0f)
		return DEFAULT_BPM;
	return out;
}





int Patch::getBars() {
	int out = atoi(getValue("bars").c_str());
	if (out <= 0 || out > 32)
		return DEFAULT_BARS;
	return out;
}





int Patch::getBeats() {
	int out = atoi(getValue("beats").c_str());
	if (out <= 0 || out > 32)
		return DEFAULT_BEATS;
	return out;
}





int Patch::getQuantize() {
	int out = atoi(getValue("quantize").c_str());
	if (out < 0 || out > 8)
		return DEFAULT_QUANTIZE;
	return out;
}





bool Patch::getMetronome() {
	bool out = atoi(getValue("metronome").c_str());
	if (out != true || out != false)
		return false;
	return out;
}





int Patch::getLastTakeId() {
	return atoi(getValue("lastTakeId").c_str());
}





int Patch::getSamplerate() {
	int out = atoi(getValue("samplerate").c_str());
	if (out <= 0)
		return DEFAULT_SAMPLERATE;
	return out;
}





uint32_t Patch::getMidiValue(int i, const char *c) {
	char tmp[32];
	sprintf(tmp, "chanMidi%s%d", c, i);
	return strtoul(getValue(tmp).c_str(), NULL, 10);
}





int Patch::readRecs() {

	puts("[PATCH] Reading recs...");

	unsigned numrecs = atoi(getValue("numrecs").c_str());

	for (unsigned i=0; i<numrecs; i++) {
		int frame, recPerFrame;

		

		char tmpbuf[16];
		sprintf(tmpbuf, "recframe%d", i);
		sscanf(getValue(tmpbuf).c_str(), "%d %d", &frame, &recPerFrame);



		for (int k=0; k<recPerFrame; k++) {
			int      chan = 0;
			int      type = 0;
			float    fValue = 0.0f;
			int      iValue_fix = 0;
			uint32_t iValue = 0;

			

			char tmpbuf[16];
			sprintf(tmpbuf, "f%da%d", i, k);

			if (version < 0.61f)    
				sscanf(getValue(tmpbuf).c_str(), "%d|%d", &chan, &type);
			else
				if (version < 0.83f)  
					sscanf(getValue(tmpbuf).c_str(), "%d|%d|%f|%d", &chan, &type, &fValue, &iValue_fix);
				else
					sscanf(getValue(tmpbuf).c_str(), "%d|%d|%f|%u", &chan, &type, &fValue, &iValue);



			Channel *ch = G_Mixer.getChannelByIndex(chan);
			if (ch)
				if (ch->status & ~(STATUS_WRONG | STATUS_MISSING | STATUS_EMPTY)) {
					if (version < 0.83f)
						recorder::rec(ch->index, type, frame, iValue_fix, fValue);
					else
						recorder::rec(ch->index, type, frame, iValue, fValue);
				}
		}
	}
	return 1;
}





#ifdef WITH_VST
int Patch::readPlugins() {
	puts("[PATCH] Reading plugins...");

	int globalOut = 1;

	

	globalOut &= readMasterPlugins(PluginHost::MASTER_IN);
	globalOut &= readMasterPlugins(PluginHost::MASTER_OUT);

	

	for (unsigned i=0; i<G_Mixer.channels.size; i++) {
		Channel *ch = G_Mixer.channels.at(i);

		char tmp[MAX_LINE_LEN];
		sprintf(tmp, "chan%dPlugins", ch->index);
		int np = atoi(getValue(tmp).c_str());

		for (int j=0; j<np; j++) {
			sprintf(tmp, "chan%d_p%dpathfile", ch->index, j);
			int out = G_PluginHost.addPlugin(getValue(tmp).c_str(), PluginHost::CHANNEL, ch);
			if (out != 0) {
				sprintf(tmp, "chan%d_p%dnumParams", ch->index, j);
				int nparam = atoi(getValue(tmp).c_str());
				Plugin *pPlugin = G_PluginHost.getPluginByIndex(j, PluginHost::CHANNEL, ch);
				sprintf(tmp, "chan%d_p%dbypass", ch->index, j);
				pPlugin->bypass = atoi(getValue(tmp).c_str());
				for (int k=0; k<nparam; k++) {
					sprintf(tmp, "chan%d_p%dparam%dvalue", ch->index, j, k);
					float pval = atof(getValue(tmp).c_str());
					pPlugin->setParam(k, pval);
				}
			}
			globalOut &= out;
		}
	}
	return globalOut;
}
#endif





int Patch::write(const char *file, const char *name, bool project) {
	fp = fopen(file, "w");
	if (fp == NULL)
		return 0;

	fprintf(fp, "# --- Giada patch file --- \n");
	fprintf(fp, "header=GIADAPTC\n");
	fprintf(fp, "version=%s\n",    VERSIONE);
	fprintf(fp, "versionf=%f\n",   VERSIONE_FLOAT);
	fprintf(fp, "patchname=%s\n",  name);
	fprintf(fp, "bpm=%f\n",        G_Mixer.bpm);
	fprintf(fp, "bars=%d\n",       G_Mixer.bars);
	fprintf(fp, "beats=%d\n",      G_Mixer.beats);
	fprintf(fp, "quantize=%d\n",   G_Mixer.quantize);
	fprintf(fp, "outVol=%f\n",     G_Mixer.outVol);
	fprintf(fp, "inVol=%f\n",      G_Mixer.inVol);
	fprintf(fp, "metronome=%d\n",  G_Mixer.metronome);
	fprintf(fp, "lastTakeId=%d\n", lastTakeId);
	fprintf(fp, "samplerate=%d\n", G_Conf.samplerate);	
	fprintf(fp, "channels=%d\n",   G_Mixer.channels.size);

	for (unsigned i=0; i<G_Mixer.channels.size; i++) {
		fprintf(fp, "# --- channel %d --- \n", i);
		G_Mixer.channels.at(i)->writePatch(fp, i, project);
	}

	

	fprintf(fp, "# --- actions --- \n");
	fprintf(fp, "numrecs=%d\n", recorder::global.size);
	for (unsigned i=0; i<recorder::global.size; i++) {
		fprintf(fp, "recframe%d=%d %d\n", i, recorder::frames.at(i), recorder::global.at(i).size);
		for (unsigned k=0; k<recorder::global.at(i).size; k++) {
			fprintf(fp, "f%da%d=%d|%d|%f|%u\n",
				i, k,
				recorder::global.at(i).at(k)->chan,
				recorder::global.at(i).at(k)->type,
				recorder::global.at(i).at(k)->fValue,
				recorder::global.at(i).at(k)->iValue);
		}
	}

#ifdef WITH_VST

	

	writeMasterPlugins(PluginHost::MASTER_IN);
	writeMasterPlugins(PluginHost::MASTER_OUT);

	

	int numPlugs;
	int numParams;
	Plugin *pPlugin;

	fprintf(fp, "# --- VST / channels --- \n");
	for (unsigned i=0; i<G_Mixer.channels.size; i++) {
		Channel *ch = G_Mixer.channels.at(i);
		numPlugs    = G_PluginHost.countPlugins(PluginHost::CHANNEL, ch);
		fprintf(fp, "chan%dPlugins=%d\n", ch->index, numPlugs);

		for (int j=0; j<numPlugs; j++) {
			pPlugin = G_PluginHost.getPluginByIndex(j, PluginHost::CHANNEL, ch);
			fprintf(fp, "chan%d_p%dpathfile=%s\n", ch->index, j, pPlugin->pathfile);
			fprintf(fp, "chan%d_p%dbypass=%d\n",   ch->index, j, pPlugin->bypass);
			numParams = pPlugin->getNumParams();
			fprintf(fp, "chan%d_p%dnumParams=%d\n", ch->index, j, numParams);

			for (int k=0; k<numParams; k++)
				fprintf(fp, "chan%d_p%dparam%dvalue=%f\n", ch->index, j, k, pPlugin->getParam(k));
		}
	}

#endif

	fclose(fp);
	return 1;
}




#ifdef WITH_VST

int Patch::readMasterPlugins(int type) {

	int  nmp;
	char chr;
	int  res = 1;

	if (type == PluginHost::MASTER_IN) {
		chr = 'I';
		nmp = atoi(getValue("masterIPlugins").c_str());
	}
	else {
		chr = 'O';
		nmp = atoi(getValue("masterOPlugins").c_str());
	}

	for (int i=0; i<nmp; i++) {
		char tmp[MAX_LINE_LEN];
		sprintf(tmp, "master%c_p%dpathfile", chr, i);
		int out = G_PluginHost.addPlugin(getValue(tmp).c_str(), type);
		if (out != 0) {
			Plugin *pPlugin = G_PluginHost.getPluginByIndex(i, type);
			sprintf(tmp, "master%c_p%dbypass", chr, i);
			pPlugin->bypass = atoi(getValue(tmp).c_str());
			sprintf(tmp, "master%c_p%dnumParams", chr, i);
			int nparam = atoi(getValue(tmp).c_str());
			for (int j=0; j<nparam; j++) {
				sprintf(tmp, "master%c_p%dparam%dvalue", chr, i, j);
				float pval = atof(getValue(tmp).c_str());
				pPlugin->setParam(j, pval);
			}
		}
		res &= out;
	}

	return res;
}





void Patch::writeMasterPlugins(int type) {

	char chr;

	if (type == PluginHost::MASTER_IN) {
		fprintf(fp, "# --- VST / master in --- \n");
		chr = 'I';
	}
	else {
		fprintf(fp, "# --- VST / master out --- \n");
		chr = 'O';
	}

	int nmp = G_PluginHost.countPlugins(type);
	fprintf(fp, "master%cPlugins=%d\n", chr, nmp);

	for (int i=0; i<nmp; i++) {
		Plugin *pPlugin = G_PluginHost.getPluginByIndex(i, type);
		fprintf(fp, "master%c_p%dpathfile=%s\n", chr, i, pPlugin->pathfile);
		fprintf(fp, "master%c_p%dbypass=%d\n", chr, i, pPlugin->bypass);
		int numParams = pPlugin->getNumParams();
		fprintf(fp, "master%c_p%dnumParams=%d\n", chr, i, numParams);

		for (int j=0; j<numParams; j++)
			fprintf(fp, "master%c_p%dparam%dvalue=%f\n", chr, i, j, pPlugin->getParam(j));
	}
}

#endif
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */






#ifdef WITH_VST

#include "plugin.h"


int Plugin::id_generator = 0;





Plugin::Plugin()
	: module    (NULL),
	  entryPoint(NULL),
	  plugin    (NULL),
	  id        (id_generator++),
	  program   (-1),
	  bypass    (false),
	  suspended (false)
{}





Plugin::~Plugin() {
	unload();
}





int Plugin::unload() {

	if (module == NULL)
		return 1;

#if defined(_WIN32)

	FreeLibrary((HMODULE)module); 
	return 1;

#elif defined(__linux__)

	return dlclose(module) == 0 ? 1 : 0;

#elif defined(__APPLE__)

	

	CFIndex retainCount = CFGetRetainCount(module);

	if (retainCount == 1) {
		puts("[plugin] retainCount == 1, can unload dlyb");
		CFBundleUnloadExecutable(module);
		CFRelease(module);
	}
	else
		printf("[plugin] retainCount > 1 (%d), leave dlyb alone\n", (int) retainCount);

	return 1;

#endif
}





int Plugin::load(const char *fname) {

	strcpy(pathfile, fname);

#if defined(_WIN32)

	module = LoadLibrary(pathfile);

#elif defined(__linux__)

	module = dlopen(pathfile, RTLD_LAZY);

#elif defined(__APPLE__)

  

  CFStringRef pathStr   = CFStringCreateWithCString(NULL, pathfile, kCFStringEncodingASCII);
  CFURLRef    bundleUrl = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,	pathStr, kCFURLPOSIXPathStyle, true);
  if(bundleUrl == NULL) {
    puts("[plugin] unable to create URL reference for plugin");
    status = 0;
    return 0;
  }
  module = CFBundleCreate(kCFAllocatorDefault, bundleUrl);

#endif

	if (module) {

	

#ifdef __APPLE__
		CFRelease(pathStr);
		CFRelease(bundleUrl);
#endif
		
		status = 1;
		return 1;
	}
	else {

#if defined(_WIN32)

		printf("[plugin] unable to load %s, error: %d\n", fname, (int) GetLastError());

#elif defined(__linux__)

		printf("[plugin] unable to load %s, error: %s\n", fname, dlerror());

#elif defined(__APPLE__)

    puts("[plugin] unable to create bundle reference");
    CFRelease(pathStr);
    CFRelease(bundleUrl);

#endif
		status = 0;
		return 0;
	}
}





int Plugin::init(VstIntPtr VSTCALLBACK (*HostCallback) (AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)) {

#if defined(_WIN32)

	entryPoint = (vstPluginFuncPtr) GetProcAddress((HMODULE)module, "VSTPluginMain");
	if (!entryPoint)
		entryPoint = (vstPluginFuncPtr) GetProcAddress((HMODULE)module, "main");

#elif defined(__linux__)

	

	void *tmp;
	tmp = dlsym(module, "VSTPluginMain");
	if (!tmp)
		tmp = dlsym(module, "main");
	memcpy(&entryPoint, &tmp, sizeof(tmp));

#elif defined(__APPLE__)

	

	void *tmp = NULL;
	tmp = CFBundleGetFunctionPointerForName(module, CFSTR("VSTPluginMain"));

	if (!tmp) {
		puts("[plugin] entryPoint 'VSTPluginMain' not found");
		tmp = CFBundleGetFunctionPointerForName(module, CFSTR("main_macho"));  
	}
	if (!tmp) {
		puts("[plugin] entryPoint 'main_macho' not found");
		tmp = CFBundleGetFunctionPointerForName(module, CFSTR("main"));
	}
	if (tmp)
		memcpy(&entryPoint, &tmp, sizeof(tmp));
	else
		puts("[plugin] entryPoint 'main' not found");

#endif

	

	if (entryPoint) {
		puts("[plugin] entryPoint found");
		plugin = entryPoint(HostCallback);
		if (!plugin) {
			puts("[plugin] failed to create effect instance!");
			return 0;
		}
	}
	else {
		puts("[plugin] entryPoint not found, unable to proceed");
		return 0;
	}


	
	

  if(plugin->magic == kEffectMagic) {
		puts("[plugin] magic number OK");
		return 1;
	}
	else {
    puts("[plugin] magic number is bad");
    return 0;
  }
}





int Plugin::setup(int samplerate, int frames) {

  

  plugin->dispatcher(plugin, effOpen, 0, 0, 0, 0);
	plugin->dispatcher(plugin, effSetSampleRate, 0, 0, 0, samplerate);
	plugin->dispatcher(plugin, effSetBlockSize, 0, frames, 0, 0);

	

	if (getSDKVersion() != kVstVersion)
		printf("[plugin] warning: different VST version (host: %d, plugin: %d)\n", kVstVersion, getSDKVersion());

	return 1;
}





AEffect *Plugin::getPlugin() {
	return plugin;
}





int Plugin::getId() { return id; }




int Plugin::getSDKVersion() {
	return plugin->dispatcher(plugin, effGetVstVersion, 0, 0, 0, 0);
}





void Plugin::getName(char *out) {
	char tmp[128] = "\0";
	plugin->dispatcher(plugin, effGetEffectName, 0, 0, tmp, 0);
	tmp[kVstMaxEffectNameLen-1] = '\0';
	strncpy(out, tmp, kVstMaxEffectNameLen);
}





void Plugin::getVendor(char *out) {
	char tmp[128] = "\0";
	plugin->dispatcher(plugin, effGetVendorString, 0, 0, tmp, 0);
	tmp[kVstMaxVendorStrLen-1] = '\0';
	strncpy(out, tmp, kVstMaxVendorStrLen);
}





void Plugin::getProduct(char *out) {
	char tmp[128] = "\0";
	plugin->dispatcher(plugin, effGetProductString, 0, 0, tmp, 0);
	tmp[kVstMaxProductStrLen-1] = '\0';
	strncpy(out, tmp, kVstMaxProductStrLen);
}





int Plugin::getNumPrograms() { return plugin->numPrograms; }





int Plugin::setProgram(int index) {
	plugin->dispatcher(plugin, effBeginSetProgram, 0, 0, 0, 0);
	plugin->dispatcher(plugin, effSetProgram, 0, index, 0, 0);
	printf("[plugin] program changed, index %d\n", index);
	program = index;
	return plugin->dispatcher(plugin, effEndSetProgram, 0, 0, 0, 0);
}





int Plugin::getNumParams() { return plugin->numParams; }





int Plugin::getNumInputs() { return plugin->numInputs; }





int Plugin::getNumOutputs() {	return plugin->numOutputs; }





void Plugin::getProgramName(int index, char *out) {
	char tmp[128] = "\0";
	plugin->dispatcher(plugin, effGetProgramNameIndexed, index, 0, tmp, 0);
	tmp[kVstMaxProgNameLen-1] = '\0';
	strncpy(out, tmp, kVstMaxProgNameLen);
}





void Plugin::getParamName(int index, char *out) {
	char tmp[128] = "\0";
	plugin->dispatcher(plugin, effGetParamName, index, 0, tmp, 0);
	tmp[kVstMaxParamStrLen-1] = '\0';
	strncpy(out, tmp, kVstMaxParamStrLen);
}





void Plugin::getParamLabel(int index, char *out) {
	char tmp[128] = "\0";
	plugin->dispatcher(plugin, effGetParamLabel, index, 0, tmp, 0);
	tmp[kVstMaxParamStrLen-1] = '\0';
	strncpy(out, tmp, kVstMaxParamStrLen);
}





void Plugin::getParamDisplay(int index, char *out) {
	char tmp[128] = "\0";
	plugin->dispatcher(plugin, effGetParamDisplay, index, 0, tmp, 0);
	tmp[kVstMaxParamStrLen-1] = '\0';
	strncpy(out, tmp, kVstMaxParamStrLen);
}





float Plugin::getParam(int index) {
	return plugin->getParameter(plugin, index);
}





void Plugin::setParam(int index, float value) {
	plugin->setParameter(plugin, index, value);
}





bool Plugin::hasGui() {
	return plugin->flags & effFlagsHasEditor;
}





void Plugin::openGui(void *w) {
	long val = 0;
#ifdef __linux__
  val = (long) w;
#endif
	plugin->dispatcher(plugin, effEditOpen, 0, val, w, 0);
}





void Plugin::closeGui() {
	plugin->dispatcher(plugin, effEditClose, 0, 0, 0, 0);
}





int Plugin::getGuiWidth() {
	ERect *pErect = NULL;
	plugin->dispatcher(plugin, effEditGetRect, 0, 0, &pErect, 0);
	return pErect->top + pErect->right;
}





int Plugin::getGuiHeight() {
	ERect *pErect = NULL;
	plugin->dispatcher(plugin, effEditGetRect, 0, 0, &pErect, 0);
	return pErect->top + pErect->bottom;
}





void Plugin::idle() {
	plugin->dispatcher(plugin, effEditIdle, 0, 0, NULL, 0);
}





void Plugin::processAudio(float **in, float **out, long frames) {
	plugin->processReplacing(plugin, in, out, frames);
}





void Plugin::processEvents(VstEvents *events) {
	plugin->dispatcher(plugin, effProcessEvents, 0, 0, events, 0.0);
}





void Plugin::resume() {
	plugin->dispatcher(plugin, effMainsChanged, 0, 1, 0, 0);
	suspended = false;
}





void Plugin::suspend() {
	plugin->dispatcher(plugin, effMainsChanged, 0, 0, 0, 0);
	suspended = true;
}





void Plugin::close() {
	plugin->dispatcher(plugin, effClose, 0, 0, 0, 0);
}





void Plugin::getRect(ERect **out) {
	plugin->dispatcher(plugin, effEditGetRect, 0, 0, out, 0);
}


#endif
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#ifdef WITH_VST


#include "pluginHost.h"
#include "conf.h"
#include "const.h"
#include "mixer.h"
#include "gd_mainWindow.h"
#include "channel.h"
#include "sampleChannel.h"
#include "midiChannel.h"
#include "kernelMidi.h"


extern Conf          G_Conf;
extern Mixer         G_Mixer;
extern PluginHost    G_PluginHost;
extern unsigned      G_beats;
extern gdMainWindow *mainWin;


PluginHost::PluginHost() {

	

	vstTimeInfo.samplePos          = 0.0;
	vstTimeInfo.sampleRate         = G_Conf.samplerate;
	vstTimeInfo.nanoSeconds        = 0.0;
	vstTimeInfo.ppqPos             = 0.0;
	vstTimeInfo.tempo              = 120.0;
	vstTimeInfo.barStartPos        = 0.0;
	vstTimeInfo.cycleStartPos      = 0.0;
	vstTimeInfo.cycleEndPos        = 0.0;
	vstTimeInfo.timeSigNumerator   = 4;
	vstTimeInfo.timeSigDenominator = 4;
	vstTimeInfo.smpteOffset        = 0;
	vstTimeInfo.smpteFrameRate     = 1;
	vstTimeInfo.samplesToNextClock = 0;
	vstTimeInfo.flags              = 0;
}





PluginHost::~PluginHost() {}





int PluginHost::allocBuffers() {

	

	

	int bufSize = kernelAudio::realBufsize*sizeof(float);

	bufferI    = (float **) malloc(2 * sizeof(float*));
	bufferI[0] =  (float *) malloc(bufSize);
	bufferI[1] =  (float *) malloc(bufSize);

	bufferO    = (float **) malloc(2 * sizeof(float*));
	bufferO[0] =  (float *) malloc(bufSize);
	bufferO[1] =  (float *) malloc(bufSize);

	memset(bufferI[0], 0, bufSize);
	memset(bufferI[1], 0, bufSize);
	memset(bufferO[0], 0, bufSize);
	memset(bufferO[1], 0, bufSize);

	printf("[pluginHost] buffers allocated, buffersize = %d\n", 2*kernelAudio::realBufsize);

	

	return 1;
}





VstIntPtr VSTCALLBACK PluginHost::HostCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt) {
	return G_PluginHost.gHostCallback(effect, opcode, index, value, ptr, opt);
}





VstIntPtr PluginHost::gHostCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt) {

	

	switch (opcode) {

		

		case audioMasterAutomate:
			return 0;

		

		case audioMasterVersion:
			return kVstVersion;

		

		case audioMasterIdle:
			return 0;

		

		case DECLARE_VST_DEPRECATED(audioMasterWantMidi):
			return 0;

		

		case audioMasterGetTime:
			vstTimeInfo.samplePos          = G_Mixer.actualFrame;
			vstTimeInfo.sampleRate         = G_Conf.samplerate;
			vstTimeInfo.tempo              = G_Mixer.bpm;
			vstTimeInfo.timeSigNumerator   = G_Mixer.beats;
			vstTimeInfo.timeSigDenominator = G_Mixer.bars;
			vstTimeInfo.ppqPos             = (G_Mixer.actualFrame / (float) G_Conf.samplerate) * (float) G_Mixer.bpm / 60.0f;
			return (VstIntPtr) &vstTimeInfo;

		

		case audioMasterProcessEvents:
			return 0;

		

		case audioMasterIOChanged:
			return false;

		

		case DECLARE_VST_DEPRECATED(audioMasterNeedIdle):
			return 0;

		

		case audioMasterSizeWindow: {
			gWindow *window = NULL;
			for (unsigned i=0; i<masterOut.size && !window; i++)
				if (masterOut.at(i)->getPlugin() == effect)
					window = masterOut.at(i)->window;

			for (unsigned i=0; i<masterIn.size && !window; i++)
				if (masterIn.at(i)->getPlugin() == effect)
					window = masterIn.at(i)->window;

			for (unsigned i=0; i<G_Mixer.channels.size && !window; i++) {
				Channel *ch = G_Mixer.channels.at(i);
				for (unsigned j=0; j<ch->plugins.size && !window; j++)
					if (ch->plugins.at(j)->getPlugin() == effect)
						window = ch->plugins.at(j)->window;
			}

			if (window) {
				printf("[pluginHost] audioMasterSizeWindow: resizing window from plugin %p\n", (void*) effect);
				if (index == 1 || value == 1)
					puts("[pluginHost] warning: non-sense values!");
				else
					window->size((int)index, (int)value);
				return 1;
			}
			else {
				printf("[pluginHost] audioMasterSizeWindow: window from plugin %p not found\n", (void*) effect);
				return 0;
			}
		}

		

		case audioMasterGetSampleRate:
			return G_Conf.samplerate;

		

		case audioMasterGetBlockSize:
			return kernelAudio::realBufsize;

		case audioMasterGetInputLatency:
			printf("[pluginHost] requested opcode 'audioMasterGetInputLatency' (%d)\n", opcode);
			return 0;

		case audioMasterGetOutputLatency:
			printf("[pluginHost] requested opcode 'audioMasterGetOutputLatency' (%d)\n", opcode);
			return 0;

		

		case audioMasterGetCurrentProcessLevel:
			return kVstProcessLevelRealtime;

		

		case audioMasterGetVendorString:
			strcpy((char*)ptr, "Monocasual");
			return 1;

		

		case audioMasterGetProductString:
			strcpy((char*)ptr, "Giada");
			return 1;

		

		case audioMasterGetVendorVersion:
			return (int) VERSIONE_FLOAT * 100;


		

		case audioMasterCanDo:
			printf("[pluginHost] audioMasterCanDo: %s\n", (char*)ptr);
			if (!strcmp((char*)ptr, "sizeWindow")       ||
					!strcmp((char*)ptr, "sendVstTimeInfo")  ||
					!strcmp((char*)ptr, "sendVstMidiEvent") ||
					!strcmp((char*)ptr, "sendVstMidiEventFlagIsRealtime"))
				return 1; 
			else
				return 0;

		

		case audioMasterUpdateDisplay:
			return 0;

		case audioMasterGetLanguage:
			return kVstLangEnglish;

		

		case audioMasterGetAutomationState:
			printf("[pluginHost] requested opcode 'audioMasterGetAutomationState' (%d)\n", opcode);
			return 0;

		

		case audioMasterBeginEdit:
			return 0;

		

		case audioMasterEndEdit:
			return 0;

		default:
			printf("[pluginHost] FIXME: host callback called with opcode %d\n", opcode);
			return 0;
	}
}





int PluginHost::addPlugin(const char *fname, int stackType, Channel *ch) {

	Plugin *p    = new Plugin();
	bool success = true;

	gVector <Plugin *> *pStack;
	pStack = getStack(stackType, ch);

	if (!p->load(fname)) {
		
		
		success = false;
	}

	

	if (!success) {
		pStack->add(p);
		return 0;
	}

	

	else {

		

		if (!p->init(&PluginHost::HostCallback)) {
			delete p;
			return 0;
		}

		

		p->setup(G_Conf.samplerate, kernelAudio::realBufsize);

		

		int lockStatus;
		while (true) {
			lockStatus = pthread_mutex_trylock(&G_Mixer.mutex_plugins);
			if (lockStatus == 0) {
				pStack->add(p);
				pthread_mutex_unlock(&G_Mixer.mutex_plugins);
				break;
			}
		}

		char name[256]; p->getName(name);
		printf("[pluginHost] plugin id=%d loaded (%s), stack type=%d, stack size=%d\n", p->getId(), name, stackType, pStack->size);

		

		p->resume();

		return 1;
	}
}





void PluginHost::processStack(float *buffer, int stackType, Channel *ch) {

	gVector <Plugin *> *pStack = getStack(stackType, ch);

	
	

	if (!G_Mixer.ready)
		return;
	if (pStack == NULL)
		return;
	if (pStack->size == 0)
		return;

	

	for (unsigned i=0; i<kernelAudio::realBufsize; i++) {
		bufferI[0][i] = buffer[i*2];
		bufferI[1][i] = buffer[(i*2)+1];
	}

	

	for (unsigned i=0; i<pStack->size; i++) {
		

		if (pStack->at(i)->status != 1)
			continue;
		if (pStack->at(i)->suspended)
			continue;
		if (pStack->at(i)->bypass)
			continue;
		if (ch) {   
			if (ch->type == CHANNEL_MIDI) {
				
				pStack->at(i)->processEvents(((MidiChannel*)ch)->getVstEvents());
			}
		}
		pStack->at(i)->processAudio(bufferI, bufferO, kernelAudio::realBufsize);
		bufferI = bufferO;
	}

	

	for (unsigned i=0; i<kernelAudio::realBufsize; i++) {
		buffer[i*2]     = bufferO[0][i];
		buffer[(i*2)+1] = bufferO[1][i];
	}
}





void PluginHost::processStackOffline(float *buffer, int stackType, Channel *ch, int size) {

	

	

	int index = 0;
	int step  = kernelAudio::realBufsize*2;

	while (index <= size) {
		int left = index+step-size;
		if (left < 0)
			processStack(&buffer[index], stackType, ch);

	

		
		

		index+=step;
	}

}





Plugin *PluginHost::getPluginById(int id, int stackType, Channel *ch) {
	gVector <Plugin *> *pStack = getStack(stackType, ch);
	for (unsigned i=0; i<pStack->size; i++) {
		if (pStack->at(i)->getId() == id)
			return pStack->at(i);
	}
	return NULL;
}





Plugin *PluginHost::getPluginByIndex(int index, int stackType, Channel *ch) {
	gVector <Plugin *> *pStack = getStack(stackType, ch);
	if (pStack->size == 0)
		return NULL;
	if ((unsigned) index >= pStack->size)
		return NULL;
	return pStack->at(index);
}





void PluginHost::freeStack(int stackType, Channel *ch) {

	gVector <Plugin *> *pStack;
	pStack = getStack(stackType, ch);

	if (pStack->size == 0)
		return;

	int lockStatus;
	while (true) {
		lockStatus = pthread_mutex_trylock(&G_Mixer.mutex_plugins);
		if (lockStatus == 0) {
			for (unsigned i=0; i<pStack->size; i++) {
				if (pStack->at(i)->status == 1) {  
					pStack->at(i)->suspend();
					pStack->at(i)->close();
				}
				delete pStack->at(i);
			}
			pStack->clear();
			pthread_mutex_unlock(&G_Mixer.mutex_plugins);
			break;
		}
	}

}





void PluginHost::freeAllStacks() {
	freeStack(PluginHost::MASTER_OUT);
	freeStack(PluginHost::MASTER_IN);
	for (unsigned i=0; i<G_Mixer.channels.size; i++)
		freeStack(PluginHost::CHANNEL, G_Mixer.channels.at(i));
}





void PluginHost::freePlugin(int id, int stackType, Channel *ch) {

	gVector <Plugin *> *pStack;
	pStack = getStack(stackType, ch);

	

	for (unsigned i=0; i<pStack->size; i++)
		if (pStack->at(i)->getId() == id) {

			if (pStack->at(i)->status == 0) { 
				delete pStack->at(i);
				pStack->del(i);
				return;
			}
			else {
				int lockStatus;
				while (true) {
					lockStatus = pthread_mutex_trylock(&G_Mixer.mutex_plugins);
					if (lockStatus == 0) {
						pStack->at(i)->suspend();
						pStack->at(i)->close();
						delete pStack->at(i);
						pStack->del(i);
						pthread_mutex_unlock(&G_Mixer.mutex_plugins);
						printf("[pluginHost] plugin id=%d removed\n", id);
						return;
					}
					
						
				}
			}
		}
	printf("[pluginHost] plugin id=%d not found\n", id);
}





void PluginHost::swapPlugin(unsigned indexA, unsigned indexB, int stackType, Channel *ch) {

	gVector <Plugin *> *pStack = getStack(stackType, ch);

	int lockStatus;
	while (true) {
		lockStatus = pthread_mutex_trylock(&G_Mixer.mutex_plugins);
		if (lockStatus == 0) {
			pStack->swap(indexA, indexB);
			pthread_mutex_unlock(&G_Mixer.mutex_plugins);
			printf("[pluginHost] plugin at index %d and %d swapped\n", indexA, indexB);
			return;
		}
		
			
	}
}





int PluginHost::getPluginIndex(int id, int stackType, Channel *ch) {

	gVector <Plugin *> *pStack = getStack(stackType, ch);

	for (unsigned i=0; i<pStack->size; i++)
		if (pStack->at(i)->getId() == id)
			return i;
	return -1;
}





gVector <Plugin *> *PluginHost::getStack(int stackType, Channel *ch) {
	switch(stackType) {
		case MASTER_OUT:
			return &masterOut;
		case MASTER_IN:
			return &masterIn;
		case CHANNEL:
			return &ch->plugins;
		default:
			return NULL;
	}
}





VstMidiEvent *PluginHost::createVstMidiEvent(uint32_t msg) {

	VstMidiEvent *e = (VstMidiEvent*) malloc(sizeof(VstMidiEvent));

	

	e->type     = kVstMidiType;
	e->byteSize = sizeof(VstMidiEvent);

	

	e->deltaFrames = 0;

	

	e->flags = kVstMidiEventIsRealtime;

	

	e->midiData[0] = kernelMidi::getB1(msg); 
	e->midiData[1] = kernelMidi::getB2(msg); 
	e->midiData[2] = kernelMidi::getB3(msg); 
	e->midiData[3] = 0;

	

	e->noteLength = 0;

	

	e->noteOffset = 0;

	

	e->noteOffVelocity = 0;

	return e;
}





unsigned PluginHost::countPlugins(int stackType, Channel *ch) {
	gVector <Plugin *> *pStack = getStack(stackType, ch);
	return pStack->size;
}


#endif 
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include <math.h>
#include "recorder.h"
#include "const.h"
#include "utils.h"
#include "mixer.h"
#include "mixerHandler.h"
#include "kernelAudio.h"
#include "pluginHost.h"
#include "kernelMidi.h"
#include "utils.h"
#include "patch.h"
#include "conf.h"
#include "channel.h"
#include "sampleChannel.h"

#ifdef WITH_VST
extern PluginHost G_PluginHost;
#endif


extern Mixer G_Mixer;
extern Patch f_patch;
extern Conf	 G_Conf;


namespace recorder {

gVector<int> frames;
gVector< gVector<action*> > global;
gVector<action*>  actions;

bool active = false;
bool sortedActions = false;

composite cmp;





void init() {
	sortedActions = false;
	active = false;
	clearAll();
}





bool canRec(Channel *ch) {

	

	if (!active || !G_Mixer.running || G_Mixer.chanInput == ch || (ch->type == CHANNEL_SAMPLE && ((SampleChannel*)ch)->wave == NULL))
		return 0;
	return 1;
}





void rec(int index, int type, int frame, uint32_t iValue, float fValue) {

	

	action *a = (action*) malloc(sizeof(action));
	a->chan   = index;
	a->type   = type;
	a->frame  = frame;
	a->iValue = iValue;
	a->fValue = fValue;

	

	int frameToExpand = frames.size;
	for (int i=0; i<frameToExpand; i++)
		if (frames.at(i) == frame) {
			frameToExpand = i;
			break;
		}

	

	if (frameToExpand == (int) frames.size) {
		frames.add(frame);
		global.add(actions);							
		global.at(global.size-1).add(a);	
	}
	else {

		

		for (unsigned t=0; t<global.at(frameToExpand).size; t++) {
			action *ac = global.at(frameToExpand).at(t);
			if (ac->chan   == index  &&
			    ac->type   == type   &&
			    ac->frame  == frame  &&
			    ac->iValue == iValue &&
			    ac->fValue == fValue)
				return;
		}

		global.at(frameToExpand).add(a);		
	}

	

#ifdef WITH_VST
	if (type == ACTION_MIDI)
		a->event = G_PluginHost.createVstMidiEvent(a->iValue);
#endif

	

	Channel *ch = G_Mixer.getChannelByIndex(index);
	ch->hasActions = true;

	sortedActions = false;

	printf("[REC] action type=%d recorded on frame=%d, chan=%d, iValue=%d (%X), fValue=%f\n",
		type, frame, index, iValue, iValue, fValue);
	
}





void clearChan(int index) {

	printf("[REC] clearing chan %d...\n", index);

	for (unsigned i=0; i<global.size; i++) {	
		unsigned j=0;
		while (true) {
			if (j == global.at(i).size) break; 	  
			action *a = global.at(i).at(j);
			if (a->chan == index)	{
#ifdef WITH_VST
				if (a->type == ACTION_MIDI)
					free(a->event);
#endif
				free(a);
				global.at(i).del(j);
			}
			else
				j++;
		}
	}

	Channel *ch = G_Mixer.getChannelByIndex(index);
	ch->hasActions = false;
	optimize();
	
}





void clearAction(int index, char act) {
	printf("[REC] clearing action %d from chan %d...\n", act, index);
	for (unsigned i=0; i<global.size; i++) {						
		unsigned j=0;
		while (true) {                                   
			if (j == global.at(i).size)
				break;
			action *a = global.at(i).at(j);
			if (a->chan == index && (act & a->type) == a->type)	{ 
				free(a);
				global.at(i).del(j);
			}
			else
				j++;
		}
	}
	Channel *ch = G_Mixer.getChannelByIndex(index);
	ch->hasActions = false;   
	optimize();
	chanHasActions(index);    
	
}





void deleteAction(int chan, int frame, char type, bool checkValues, uint32_t iValue, float fValue) {

	

	bool found = false;
	for (unsigned i=0; i<frames.size && !found; i++) {
		if (frames.at(i) == frame) {

			

			for (unsigned j=0; j<global.at(i).size; j++) {
				action *a = global.at(i).at(j);

				

				bool doit = (a->chan == chan && a->type == (type & a->type));
				if (checkValues)
					doit &= (a->iValue == iValue && a->fValue == fValue);

				if (doit) {
					int lockStatus = 0;
					while (lockStatus == 0) {
						lockStatus = pthread_mutex_trylock(&G_Mixer.mutex_recs);
						if (lockStatus == 0) {
#ifdef WITH_VST
							if (type == ACTION_MIDI)
								free(a->event);
#endif
							free(a);
							global.at(i).del(j);
							pthread_mutex_unlock(&G_Mixer.mutex_recs);
							found = true;
							break;
						}
						else
							puts("[REC] delete action: waiting for mutex...");
					}
				}
			}
		}
	}
	if (found) {
		optimize();
		chanHasActions(chan);
		printf("[REC] action deleted, type=%d frame=%d chan=%d iValue=%d (%X) fValue=%f\n",
			type, frame, chan, iValue, iValue, fValue);
	}
	else
		printf("[REC] unable to find action! type=%d frame=%d chan=%d iValue=%d (%X) fValue=%f\n",
			type, frame, chan, iValue, iValue, fValue);
}





void deleteActions(int chan, int frame_a, int frame_b, char type) {

	sortActions();
	gVector<int> dels;

	for (unsigned i=0; i<frames.size; i++)
		if (frames.at(i) > frame_a && frames.at(i) < frame_b)
			dels.add(frames.at(i));

	for (unsigned i=0; i<dels.size; i++)
		deleteAction(chan, dels.at(i), type, false); 
}





void clearAll() {
	while (global.size > 0) {
		for (unsigned i=0; i<global.size; i++) {
			for (unsigned k=0; k<global.at(i).size; k++) {
#ifdef WITH_VST
				if (global.at(i).at(k)->type == ACTION_MIDI)
					free(global.at(i).at(k)->event);
#endif
				free(global.at(i).at(k));									
			}
			global.at(i).clear();												
			global.del(i);
		}
	}

	for (unsigned i=0; i<G_Mixer.channels.size; i++) {
		G_Mixer.channels.at(i)->hasActions  = false;
		if (G_Mixer.channels.at(i)->type == CHANNEL_SAMPLE)
			((SampleChannel*)G_Mixer.channels.at(i))->readActions = false;
	}

	global.clear();
	frames.clear();
}





void optimize() {

	

	unsigned i = 0;
	while (true) {
		if (i == global.size) return;
		if (global.at(i).size == 0) {
			global.del(i);
			frames.del(i);
		}
		else
			i++;
	}

	sortActions();
}





void sortActions() {
	if (sortedActions)
		return;
	for (unsigned i=0; i<frames.size; i++)
		for (unsigned j=0; j<frames.size; j++)
			if (frames.at(j) > frames.at(i)) {
				frames.swap(j, i);
				global.swap(j, i);
			}
	sortedActions = true;
	
}





void updateBpm(float oldval, float newval, int oldquanto) {

	for (unsigned i=0; i<frames.size; i++) {

		float frame  = ((float) frames.at(i)/newval) * oldval;
		frames.at(i) = (int) frame;

		
		

		if (frames.at(i) != 0) {
			int scarto = oldquanto % frames.at(i);
			if (scarto > 0 && scarto <= 6)
				frames.at(i) = frames.at(i) + scarto;
		}

		

		if (frames.at(i) % 2 != 0)
			frames.at(i)++;
	}

	

	for (unsigned i=0; i<frames.size; i++) {
		for (unsigned j=0; j<global.at(i).size; j++) {
			action *a = global.at(i).at(j);
			a->frame = frames.at(i);
		}
	}

	
}





void updateSamplerate(int systemRate, int patchRate) {

	

	if (systemRate == patchRate)
		return;

	printf("[REC] systemRate (%d) != patchRate (%d), converting...\n", systemRate, patchRate);

	float ratio = systemRate / (float) patchRate;
	for (unsigned i=0; i<frames.size; i++) {

		printf("[REC]    oldFrame = %d", frames.at(i));

		float newFrame = frames.at(i);
		newFrame = floorf(newFrame * ratio);

		frames.at(i) = (int) newFrame;

		if (frames.at(i) % 2 != 0)
			frames.at(i)++;

		printf(", newFrame = %d\n", frames.at(i));
	}

	

	for (unsigned i=0; i<frames.size; i++) {
		for (unsigned j=0; j<global.at(i).size; j++) {
			action *a = global.at(i).at(j);
			a->frame = frames.at(i);
		}
	}
}





void expand(int old_fpb, int new_fpb) {

	

	unsigned pass = (int) (new_fpb / old_fpb) - 1;
	if (pass == 0) pass = 1;

	unsigned init_fs = frames.size;

	for (unsigned z=1; z<=pass; z++) {
		for (unsigned i=0; i<init_fs; i++) {
			unsigned newframe = frames.at(i) + (old_fpb*z);
			frames.add(newframe);
			global.add(actions);
			for (unsigned k=0; k<global.at(i).size; k++) {
				action *a = global.at(i).at(k);
				rec(a->chan, a->type, newframe, a->iValue, a->fValue);
			}
		}
	}
	printf("[REC] expanded recs\n");
	
}





void shrink(int new_fpb) {

	

	unsigned i=0;
	while (true) {
		if (i == frames.size) break;

		if (frames.at(i) >= new_fpb) {
			for (unsigned k=0; k<global.at(i).size; k++)
				free(global.at(i).at(k));			
			global.at(i).clear();								
			global.del(i);											
			frames.del(i);											
		}
		else
			i++;
	}
	optimize();
	printf("[REC] shrinked recs\n");
	
}





void chanHasActions(int index) {
	Channel *ch = G_Mixer.getChannelByIndex(index);
	if (global.size == 0) {
		ch->hasActions = false;
		return;
	}
	for (unsigned i=0; i<global.size && !ch->hasActions; i++) {
		for (unsigned j=0; j<global.at(i).size && !ch->hasActions; j++) {
			if (global.at(i).at(j)->chan == index)
				ch->hasActions = true;
		}
	}
}





int getNextAction(int chan, char type, int frame, action **out, uint32_t iValue) {

	sortActions();  

	unsigned i=0;
	while (i < frames.size && frames.at(i) <= frame) i++;

	if (i == frames.size)   
		return -1;

	for (; i<global.size; i++)
		for (unsigned j=0; j<global.at(i).size; j++) {
			action *a = global.at(i).at(j);
			if (a->chan == chan && (type & a->type) == a->type) {
				if (iValue == 0 || (iValue != 0 && a->iValue == iValue)) {
					*out = global.at(i).at(j);
					return 1;
				}
			}
		}

	return -2;   
}





int getAction(int chan, char action, int frame, struct action **out) {
	for (unsigned i=0; i<global.size; i++)
		for (unsigned j=0; j<global.at(i).size; j++)
			if (frame  == global.at(i).at(j)->frame &&
					action == global.at(i).at(j)->type &&
					chan   == global.at(i).at(j)->chan)
			{
				*out = global.at(i).at(j);
				return 1;
			}
	return 0;
}





void startOverdub(int index, char actionMask, int frame) {

	

	if (actionMask == ACTION_KEYS) {
		cmp.a1.type = ACTION_KEYPRESS;
		cmp.a2.type = ACTION_KEYREL;
	}
	else {
		cmp.a1.type = ACTION_MUTEON;
		cmp.a2.type = ACTION_MUTEOFF;
	}
	cmp.a1.chan  = index;
	cmp.a2.chan  = index;
	cmp.a1.frame = frame;
	

	

	rec(index, cmp.a1.type, frame);

	action *act = NULL;
	int res = getNextAction(index, cmp.a1.type | cmp.a2.type, cmp.a1.frame, &act);
	if (res == 1) {
		if (act->type == cmp.a2.type) {
			int truncFrame = cmp.a1.frame-kernelAudio::realBufsize;
			if (truncFrame < 0)
				truncFrame = 0;
			printf("[REC] add truncation at frame %d, type=%d\n", truncFrame, cmp.a2.type);
			rec(index, cmp.a2.type, truncFrame);
		}
	}

	SampleChannel *ch = (SampleChannel*) G_Mixer.getChannelByIndex(index);
	ch->readActions = false;   
}





void stopOverdub(int frame) {

	cmp.a2.frame  = frame;
	bool ringLoop = false;
	bool nullLoop = false;

	

	if (cmp.a2.frame < cmp.a1.frame) {
		ringLoop = true;
		printf("[REC] ring loop! frame1=%d < frame2=%d\n", cmp.a1.frame, cmp.a2.frame);
		rec(cmp.a2.chan, cmp.a2.type, G_Mixer.totalFrames); 	
	}
	else
	if (cmp.a2.frame == cmp.a1.frame) {
		nullLoop = true;
		printf("[REC]  null loop! frame1=%d == frame2=%d\n", cmp.a1.frame, cmp.a2.frame);
		deleteAction(cmp.a1.chan, cmp.a1.frame, cmp.a1.type, false); 
	}

	SampleChannel *ch = (SampleChannel*) G_Mixer.getChannelByIndex(cmp.a2.chan);
	ch->readActions = false;      

	

	if (!nullLoop)
		deleteActions(cmp.a2.chan, cmp.a1.frame, cmp.a2.frame, cmp.a1.type);
		deleteActions(cmp.a2.chan, cmp.a1.frame, cmp.a2.frame, cmp.a2.type);

	if (!ringLoop && !nullLoop) {
		rec(cmp.a2.chan, cmp.a2.type, cmp.a2.frame);

		

		action *act = NULL;
		int res = getNextAction(cmp.a2.chan, cmp.a1.type | cmp.a2.type, cmp.a2.frame, &act);
		if (res == 1) {
			if (act->type == cmp.a2.type) {
				printf("[REC] add truncation at frame %d, type=%d\n", act->frame, act->type);
				deleteAction(act->chan, act->frame, act->type, false); 
			}
		}
	}
}





void print() {
	printf("[REC] ** print debug **\n");
	for (unsigned i=0; i<global.size; i++) {
		printf("  frame %d\n", frames.at(i));
		for (unsigned j=0; j<global.at(i).size; j++) {
			printf("    action %d | chan %d | frame %d\n", global.at(i).at(j)->type, global.at(i).at(j)->chan, global.at(i).at(j)->frame);
		}
	}
}

} 
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include <math.h>
#include "sampleChannel.h"
#include "patch.h"
#include "conf.h"
#include "wave.h"
#include "pluginHost.h"
#include "waveFx.h"
#include "mixerHandler.h"


extern Patch       G_Patch;
extern Mixer       G_Mixer;
extern Conf        G_Conf;
#ifdef WITH_VST
extern PluginHost  G_PluginHost;
#endif


SampleChannel::SampleChannel(int bufferSize, char side)
	: Channel          (CHANNEL_SAMPLE, STATUS_EMPTY, side, bufferSize),
		frameRewind      (-1),
		wave             (NULL),
		tracker          (0),
		begin            (0),
		end              (0),
		pitch            (gDEFAULT_PITCH),
		boost            (1.0f),
		mode             (DEFAULT_CHANMODE),
		qWait	           (false),
		fadeinOn         (false),
		fadeinVol        (1.0f),
		fadeoutOn        (false),
		fadeoutVol       (1.0f),
		fadeoutTracker   (0),
		fadeoutStep      (DEFAULT_FADEOUT_STEP),
		key              (0),
	  readActions      (true),
	  midiInReadActions(0x0),
	  midiInPitch      (0x0)
{
	rsmp_state = src_new(SRC_LINEAR, 2, NULL);
	pChan      = (float *) malloc(kernelAudio::realBufsize * 2 * sizeof(float));
}





SampleChannel::~SampleChannel() {
	if (wave)
		delete wave;
	src_delete(rsmp_state);
	free(pChan);
}





void SampleChannel::clear() {

	

		memset(vChan, 0, sizeof(float) * bufferSize);
		memset(pChan, 0, sizeof(float) * bufferSize);

	if (status & (STATUS_PLAY | STATUS_ENDING)) {
		tracker = fillChan(vChan, tracker, 0);
		if (fadeoutOn && fadeoutType == XFADE)
			fadeoutTracker = fillChan(pChan, fadeoutTracker, 0);
	}
}





void SampleChannel::calcVolumeEnv(int frame) {

	

	recorder::action *a0 = NULL;
	recorder::action *a1 = NULL;
	int res;

	

	res = recorder::getAction(index, ACTION_VOLUME, frame, &a0);
	if (res == 0)
		return;

	

	res = recorder::getNextAction(index, ACTION_VOLUME, frame, &a1);

	if (res == -1)
		res = recorder::getAction(index, ACTION_VOLUME, 0, &a1);

	volume_i = a0->fValue;
	volume_d = ((a1->fValue - a0->fValue) / ((a1->frame - a0->frame) / 2)) * 1.003f;
}





void SampleChannel::hardStop(int frame) {
	if (frame != 0)        
		clearChan(vChan, frame);
	status = STATUS_OFF;
	reset(frame);
}





void SampleChannel::onBar(int frame) {
	if (mode == LOOP_REPEAT && status == STATUS_PLAY)
		setXFade(frame);
}





int SampleChannel::save(const char *path) {
	return wave->writeData(path);
}





void SampleChannel::setBegin(unsigned v) {
	begin   = v;
	tracker = begin;
}





void SampleChannel::setEnd(unsigned v) {
	end = v;
}





void SampleChannel::setPitch(float v) {

	pitch = v;
	rsmp_data.src_ratio = 1/pitch;

	

	if (status & (STATUS_OFF | STATUS_WAIT))
		src_set_ratio(rsmp_state, 1/pitch);
}





void SampleChannel::rewind() {

	

	if (wave != NULL) {
		if ((mode & LOOP_ANY) || (recStatus == REC_READING && (mode & SINGLE_ANY)))
			reset(0);  
	}
}





void SampleChannel::parseAction(recorder::action *a, int localFrame, int globalFrame) {

	if (readActions == false)
		return;

	switch (a->type) {
		case ACTION_KEYPRESS:
			if (mode & SINGLE_ANY)
				start(localFrame, false);
			break;
		case ACTION_KEYREL:
			if (mode & SINGLE_ANY)
				stop();
			break;
		case ACTION_KILLCHAN:
			if (mode & SINGLE_ANY)
				kill(localFrame);
			break;
		case ACTION_MUTEON:
			setMute(true);   
			break;
		case ACTION_MUTEOFF:
			unsetMute(true); 
			break;
		case ACTION_VOLUME:
			calcVolumeEnv(globalFrame);
			break;
	}
}





void SampleChannel::sum(int frame, bool running) {

	if (wave == NULL || status & ~(STATUS_PLAY | STATUS_ENDING))
		return;

	if (frame != frameRewind) {

		

		if (running) {
			volume_i += volume_d;
			if (volume_i < 0.0f)
				volume_i = 0.0f;
			else
			if (volume_i > 1.0f)
				volume_i = 1.0f;
		}

		

		

		if (mute || mute_i) {
			vChan[frame]   = 0.0f;
			vChan[frame+1] = 0.0f;
		}
		else
		if (fadeinOn) {
			if (fadeinVol < 1.0f) {
				vChan[frame]   *= fadeinVol * volume_i;
				vChan[frame+1] *= fadeinVol * volume_i;
				fadeinVol += 0.01f;
			}
			else {
				fadeinOn  = false;
				fadeinVol = 0.0f;
			}
		}
		else
		if (fadeoutOn) {
			if (fadeoutVol >= 0.0f) { 
				if (fadeoutType == XFADE) {

					vChan[frame]   *= volume_i;
					vChan[frame+1] *= volume_i;

					vChan[frame]   += pChan[frame]   * fadeoutVol;
					vChan[frame+1] += pChan[frame+1] * fadeoutVol;

				}
				else {
					vChan[frame]   *= fadeoutVol * volume_i;
					vChan[frame+1] *= fadeoutVol * volume_i;
				}
				fadeoutVol -= fadeoutStep;
			}
			else {  
				fadeoutOn  = false;
				fadeoutVol = 1.0f;

				

				if (fadeoutType == XFADE) {
					qWait = false;
				}
				else {
					if (fadeoutEnd == DO_MUTE)
						mute = true;
					else
					if (fadeoutEnd == DO_MUTE_I)
						mute_i = true;
					else             
						hardStop(frame);
				}

				

				vChan[frame]   = vChan[frame-2];
				vChan[frame+1] = vChan[frame-1];
			}
		}
		else {
			vChan[frame]   *= volume_i;
			vChan[frame+1] *= volume_i;
		}
	}
	else {

		if (mode & (SINGLE_BASIC | SINGLE_PRESS | SINGLE_RETRIG) ||
			 (mode == SINGLE_ENDLESS && status == STATUS_ENDING)   ||
			 (mode & LOOP_ANY && !running))     
		{
			status = STATUS_OFF;
		}

		

		if (mode == LOOP_ONCE && status != STATUS_ENDING)
			status = STATUS_WAIT;

		

		reset(frame);
	}
}





void SampleChannel::onZero(int frame) {

	if (wave == NULL)
		return;

	if (mode & (LOOP_ONCE | LOOP_BASIC | LOOP_REPEAT)) {

		

		if (status == STATUS_PLAY) {
			if (mute || mute_i)
				reset(frame);
			else
				setXFade(frame);
		}
		else
		if (status == STATUS_ENDING)
			hardStop(frame);
	}

	if (status == STATUS_WAIT) { 
		status  = STATUS_PLAY;
		tracker = fillChan(vChan, tracker, frame);
	}

	if (recStatus == REC_ENDING) {
		recStatus = REC_STOPPED;
		setReadActions(false);  
	}
	else
	if (recStatus == REC_WAITING) {
		recStatus = REC_READING;
		setReadActions(true);   
	}
}





void SampleChannel::quantize(int index, int localFrame, int globalFrame) {

	

	if ((mode & LOOP_ANY) || !qWait)
		return;

	

	if (status == STATUS_OFF) {
		status  = STATUS_PLAY;
		qWait   = false;
		tracker = fillChan(vChan, tracker, localFrame);
	}
	else
		setXFade(localFrame);

	

	if (recorder::canRec(this)) {
		if (mode == SINGLE_PRESS)
			recorder::startOverdub(index, ACTION_KEYS, globalFrame);
		else
			recorder::rec(index, ACTION_KEYPRESS, globalFrame);
	}
}





int SampleChannel::getPosition() {
	if (status & ~(STATUS_EMPTY | STATUS_MISSING | STATUS_OFF)) 
		return tracker - begin;
	else
		return -1;
}





void SampleChannel::setMute(bool internal) {

	if (internal) {

		

		if (mute)
			mute_i = true;
		else {
			if (isPlaying())
				setFadeOut(DO_MUTE_I);
			else
				mute_i = true;
		}
	}
	else {

		

		if (mute_i)
			mute = true;
		else {

			

			if (isPlaying())
				setFadeOut(DO_MUTE);
			else
				mute = true;
		}
	}
}





void SampleChannel::unsetMute(bool internal) {
	if (internal) {
		if (mute)
			mute_i = false;
		else {
			if (isPlaying())
				setFadeIn(internal);
			else
				mute_i = false;
		}
	}
	else {
		if (mute_i)
			mute = false;
		else {
			if (isPlaying())
				setFadeIn(internal);
			else
				mute = false;
		}
	}
}





void SampleChannel::calcFadeoutStep() {
	if (end - tracker < (1 / DEFAULT_FADEOUT_STEP) * 2)
		fadeoutStep = ceil((end - tracker) / volume) * 2; 
	else
		fadeoutStep = DEFAULT_FADEOUT_STEP;
}





void SampleChannel::setReadActions(bool v) {
	if (v)
		readActions = true;
	else {
		readActions = false;
		if (G_Conf.recsStopOnChanHalt)
			kill(0);  
	}
}





void SampleChannel::setFadeIn(bool internal) {
	if (internal) mute_i = false;  
	else          mute   = false;
	fadeinOn  = true;
	fadeinVol = 0.0f;
}





void SampleChannel::setFadeOut(int actionPostFadeout) {
	calcFadeoutStep();
	fadeoutOn   = true;
	fadeoutVol  = 1.0f;
	fadeoutType = FADEOUT;
	fadeoutEnd	= actionPostFadeout;
}





void SampleChannel::setXFade(int frame) {
	calcFadeoutStep();
	fadeoutOn      = true;
	fadeoutVol     = 1.0f;
	fadeoutTracker = tracker;
	fadeoutType    = XFADE;
	reset(frame);
}







void SampleChannel::reset(int frame) {
	fadeoutTracker = tracker;   
	tracker = begin;
	mute_i  = false;
	if (frame > 0 && status & (STATUS_PLAY | STATUS_ENDING))
		tracker = fillChan(vChan, tracker, frame);
}





void SampleChannel::empty() {
	status = STATUS_OFF;
	if (wave) {
		delete wave;
		wave = NULL;
	}
	status = STATUS_EMPTY;
}





void SampleChannel::pushWave(Wave *w) {
	wave   = w;
	status = STATUS_OFF;
	begin  = 0;
	end    = wave->size;
}





bool SampleChannel::allocEmpty(int frames, int takeId) {

	Wave *w = new Wave();
	if (!w->allocEmpty(frames))
		return false;

	char wname[32];
	sprintf(wname, "TAKE-%d", takeId);

	w->pathfile = gGetCurrentPath()+"/"+wname;
	w->name     = wname;
	wave        = w;
	status      = STATUS_OFF;
	begin       = 0;
	end         = wave->size;

	return true;
}





void SampleChannel::process(float *buffer) {

#ifdef WITH_VST
	G_PluginHost.processStack(vChan, PluginHost::CHANNEL, this);
#endif

	for (int j=0; j<bufferSize; j+=2) {
		buffer[j]   += vChan[j]   * volume * panLeft  * boost;
		buffer[j+1] += vChan[j+1] * volume * panRight * boost;
	}
}





void SampleChannel::kill(int frame) {
	if (wave != NULL && status != STATUS_OFF) {
		if (mute || mute_i || (status == STATUS_WAIT && mode & LOOP_ANY))
			hardStop(frame);
		else
			setFadeOut(DO_STOP);
	}
}





void SampleChannel::stopBySeq() {

	

	if (!G_Conf.chansStopOnSeqHalt)
		return;

	if (mode & (LOOP_BASIC | LOOP_ONCE | LOOP_REPEAT))
		kill(0);

	

	

	if (hasActions && readActions && status == STATUS_PLAY)
		kill(0);
}





void SampleChannel::stop() {
	if (mode == SINGLE_PRESS && status == STATUS_PLAY) {
		if (mute || mute_i)
			hardStop(0);  
		else
			setFadeOut(DO_STOP);
	}
	else  
	if (mode == SINGLE_PRESS && qWait == true)
		qWait = false;
}





int SampleChannel::load(const char *file) {

	if (strcmp(file, "") == 0 || gIsDir(file)) {
		puts("[SampleChannel] file not specified");
		return SAMPLE_LEFT_EMPTY;
	}

	if (strlen(file) > FILENAME_MAX)
		return SAMPLE_PATH_TOO_LONG;

	Wave *w = new Wave();

	if (!w->open(file)) {
		printf("[SampleChannel] %s: read error\n", file);
		delete w;
		return SAMPLE_READ_ERROR;
	}

	if (w->channels() > 2) {
		printf("[SampleChannel] %s: unsupported multichannel wave\n", file);
		delete w;
		return SAMPLE_MULTICHANNEL;
	}

	if (!w->readData()) {
		delete w;
		return SAMPLE_READ_ERROR;
	}

	if (w->channels() == 1) 
		wfx_monoToStereo(w);

	if (w->rate() != G_Conf.samplerate) {
		printf("[SampleChannel] input rate (%d) != system rate (%d), conversion needed\n",
				w->rate(), G_Conf.samplerate);
		w->resample(G_Conf.rsmpQuality, G_Conf.samplerate);
	}

	pushWave(w);

	

	std::string oldName = wave->name;
	int k = 1;
	while (!mh_uniqueSamplename(this, wave->name.c_str())) {
		wave->updateName((oldName + "-" + gItoa(k)).c_str());
		k++;
	}

	printf("[SampleChannel] %s loaded in channel %d\n", file, index);
	return SAMPLE_LOADED_OK;
}





int SampleChannel::loadByPatch(const char *f, int i) {

	int res = load(f);

	if (res == SAMPLE_LOADED_OK) {
		volume      = G_Patch.getVol(i);
		key         = G_Patch.getKey(i);
		index       = G_Patch.getIndex(i);
		mode        = G_Patch.getMode(i);
		mute        = G_Patch.getMute(i);
		mute_s      = G_Patch.getMute_s(i);
		solo        = G_Patch.getSolo(i);
		boost       = G_Patch.getBoost(i);
		panLeft     = G_Patch.getPanLeft(i);
		panRight    = G_Patch.getPanRight(i);
		readActions = G_Patch.getRecActive(i);
		recStatus   = readActions ? REC_READING : REC_STOPPED;

		readPatchMidiIn(i);
		midiInReadActions = G_Patch.getMidiValue(i, "InReadActions");
		midiInPitch       = G_Patch.getMidiValue(i, "InPitch");

		setBegin(G_Patch.getBegin(i));
		setEnd  (G_Patch.getEnd(i, wave->size));
		setPitch(G_Patch.getPitch(i));
	}
	else {
		volume = DEFAULT_VOL;
		mode   = DEFAULT_CHANMODE;
		status = STATUS_WRONG;
		key    = 0;

		if (res == SAMPLE_LEFT_EMPTY)
			status = STATUS_EMPTY;
		else
		if (res == SAMPLE_READ_ERROR)
			status = STATUS_MISSING;
	}

	return res;
}





bool SampleChannel::canInputRec() {
	return wave == NULL;
}





void SampleChannel::start(int frame, bool doQuantize) {

	switch (status)	{
		case STATUS_EMPTY:
		case STATUS_MISSING:
		case STATUS_WRONG:
		{
			return;
		}

		case STATUS_OFF:
		{
			if (mode & LOOP_ANY) {
				status = STATUS_WAIT;
			}
			else {
				if (G_Mixer.quantize > 0 && G_Mixer.running && doQuantize)
					qWait = true;
				else {
					status = STATUS_PLAY;
					if (frame > 0)   
						tracker = fillChan(vChan, tracker, frame);
				}
			}
			break;
		}

		case STATUS_PLAY:
		{
			if (mode == SINGLE_BASIC)
				setFadeOut(DO_STOP);
			else
			if (mode == SINGLE_RETRIG) {
				if (G_Mixer.quantize > 0 && G_Mixer.running && doQuantize)
					qWait = true;
				else
					mute ? reset(frame) : setXFade(frame); 
			}
			else
			if (mode & (LOOP_ANY | SINGLE_ENDLESS))
				status = STATUS_ENDING;
			break;
		}

		case STATUS_WAIT:
		{
			status = STATUS_OFF;
			break;
		}

		case STATUS_ENDING:
		{
			status = STATUS_PLAY;
			break;
		}
	}
}





void SampleChannel::writePatch(FILE *fp, int i, bool isProject) {

	const char *path = "";
	if (wave != NULL) {
		path = wave->pathfile.c_str();
		if (isProject)
			path = gBasename(path).c_str();  
	}

	fprintf(fp, "samplepath%d=%s\n",     i, path);
	fprintf(fp, "chanSide%d=%d\n",       i, side);
	fprintf(fp, "chanType%d=%d\n",       i, type);
	fprintf(fp, "chanKey%d=%d\n",        i, key);
	fprintf(fp, "chanIndex%d=%d\n",      i, index);
	fprintf(fp, "chanmute%d=%d\n",       i, mute);
	fprintf(fp, "chanMute_s%d=%d\n",     i, mute_s);
	fprintf(fp, "chanSolo%d=%d\n",       i, solo);
	fprintf(fp, "chanvol%d=%f\n",        i, volume);
	fprintf(fp, "chanmode%d=%d\n",       i, mode);
	fprintf(fp, "chanBegin%d=%d\n",      i, begin);
	fprintf(fp, "chanend%d=%d\n",        i, end);
	fprintf(fp, "chanBoost%d=%f\n",      i, boost);
	fprintf(fp, "chanPanLeft%d=%f\n",    i, panLeft);
	fprintf(fp, "chanPanRight%d=%f\n",   i, panRight);
	fprintf(fp, "chanRecActive%d=%d\n",  i, readActions);
	fprintf(fp, "chanPitch%d=%f\n",      i, pitch);

	writePatchMidiIn(fp, i);
	fprintf(fp, "chanMidiInReadActions%d=%u\n", i, midiInReadActions);
	fprintf(fp, "chanMidiInPitch%d=%u\n",       i, midiInPitch);
}





void SampleChannel::clearChan(float *dest, int start) {
	memset(dest+start, 0, sizeof(float)*(bufferSize-start));
}





int SampleChannel::fillChan(float *dest, int start, int offset) {

	int position;  

	if (pitch == 1.0f) {

		

		if (start+bufferSize-offset <= end) {
			memcpy(dest+offset, wave->data+start, (bufferSize-offset)*sizeof(float));
			frameRewind = -1;
			position    = start+bufferSize-offset;
		}

		

		else {
			memcpy(dest+offset, wave->data+start, (end-start)*sizeof(float));
			frameRewind = end-start+offset;
			position    = end;
		}
	}
	else {
		rsmp_data.data_in       = wave->data+start;       
		rsmp_data.input_frames  = (end-start)/2;          
		rsmp_data.data_out      = dest+offset;           
		rsmp_data.output_frames = (bufferSize-offset)/2;  
		rsmp_data.end_of_input  = false;

		src_process(rsmp_state, &rsmp_data);
		int gen = rsmp_data.output_frames_gen*2;          

		position = start + rsmp_data.input_frames_used*2; 

		if (gen == bufferSize-offset)
			frameRewind = -1;
		else
			frameRewind = gen+offset;
	}
	return position;
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include "utils.h"
#if defined(_WIN32)			
	#include <direct.h>
	#include <windows.h>
#else
	#include <unistd.h>
#endif

#include <cstdarg>
#include <sys/stat.h>   
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <string.h>
#include <sstream>
#include <limits.h>
#if defined(__APPLE__)
	#include <libgen.h>     
#endif



bool gFileExists(const char *filename) {
	FILE *fh = fopen(filename, "rb");
	if (!fh) {
		return 0;
	}
	else {
		fclose(fh);
		return 1;
	}
}





bool gIsDir(const char *path) {
	bool ret;

#if defined(__linux__)

	struct stat s1;
	stat(path, &s1);
	ret = S_ISDIR(s1.st_mode);

#elif defined(__APPLE__)

	if (strcmp(path, "")==0)
		ret = false;
	else {
		struct stat s1;
		stat(path, &s1);
		ret = S_ISDIR(s1.st_mode);

		

		if (ret) {
			std::string tmp = path;
			tmp += "/Contents/Info.plist";
			if (gFileExists(tmp.c_str()))
				ret = false;
		}
	}

#elif defined(__WIN32)

  unsigned dwAttrib = GetFileAttributes(path);
  ret = (dwAttrib != INVALID_FILE_ATTRIBUTES &&
        (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#endif

	return ret & !gIsProject(path);
}





bool gDirExists(const char *path) {
	struct stat st;
	if (stat(path, &st) != 0 && errno == ENOENT)
		return false;
	return true;
}





bool gMkdir(const char *path) {
#if defined(__linux__) || defined(__APPLE__)
	if (mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)
#else
	if (_mkdir(path) == 0)
#endif
		return true;
	return false;
}





std::string gBasename(const char *fullpath) {
#if defined(__linux__)
	const char *buffer;
	buffer = basename(fullpath);
#elif defined(__APPLE__)
	char path[PATH_MAX];
	sprintf(path, "%s", fullpath);
	char *buffer = basename(path);
#elif defined(_WIN32)
	char file[_MAX_FNAME];
	char ext[_MAX_EXT];
	char buffer[FILENAME_MAX];
	_splitpath(fullpath, NULL, NULL, file, ext);
	sprintf(buffer, "%s.%s", file, ext);
#endif
	std::string out = buffer;
	return out;
}





std::string gDirname(const char *path) {
	std::string base = gBasename(path);
	std::string full = path;
	size_t i = full.find(base);
	return full.erase(i-1, full.size());  
}






std::string gGetCurrentPath() {
 char buf[PATH_MAX];
#if defined(__WIN32)
	if (_getcwd(buf, PATH_MAX) != NULL)
#else
	if (getcwd(buf, PATH_MAX) != NULL)
#endif
		return buf;
	else
		return "";
}





std::string gGetExt(const char *file) {
	int len = strlen(file);
	int pos = len;
	while (pos>0) {
		if (file[pos] == '.')
			break;
		pos--;
	}
	if (pos==0)
		return "";
	std::string out = file;
	return out.substr(pos+1, len);
}





std::string gStripExt(const char *file) {
	int len = strlen(file);
	int pos = -1;
	for (int i=0; i<len; i++)
		if (file[i] == '.') {
			pos = i;
			break;
		}
	std::string out = file;
	return pos == -1 ? out : out.substr(0, pos);
}





bool gIsProject(const char *path) {

	

	if (gGetExt(path) == "gprj" && gDirExists(path))
		return 1;
	return 0;
}





bool gIsPatch(const char *path) {
	if (gGetExt(path) == "gptc")
		return 1;
	return 0;
}





std::string gGetProjectName(const char *path) {
	std::string out;
	out = gStripExt(path);

	int i = out.size();
	while (i>=0) {
#if defined(__linux__) || defined(__APPLE__)
		if (out[i] == '/')
#elif defined(_WIN32)
		if (out[i] == '\\')
#endif
			break;
		i--;
	}

	out.erase(0, i+1);	
	return out;
}





std::string gGetSlash() {
#if defined(_WIN32)
	return "\\";
#else
	return "/";
#endif
}





std::string gItoa(int i) {
	std::stringstream out;
	out << i;
	return out.str();
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include <stdio.h>
#include <stdlib.h>
#include <string.h>  
#include <math.h>
#include "wave.h"
#include "utils.h"
#include "conf.h"
#include "init.h"


extern Conf G_Conf;


Wave::Wave() : data(NULL), size(0), isLogical(0), isEdited(0) {}





Wave::~Wave() {
	clear();
}





int Wave::open(const char *f) {

	pathfile = f;
	name     = gStripExt(gBasename(f).c_str());
	fileIn   = sf_open(f, SFM_READ, &inHeader);

	if (fileIn == NULL) {
		printf("[wave] unable to read %s. %s\n", f, sf_strerror(fileIn));
		pathfile = "";
		name     = "";
		return 0;
	}

	isLogical = false;
	isEdited  = false;

	return 1;
}






int Wave::readData() {
	size = inHeader.frames * inHeader.channels;
	data = (float *) malloc(size * sizeof(float));
	if (data == NULL) {
		puts("[wave] unable to allocate memory");
		return 0;
	}

	if (sf_read_float(fileIn, data, size) != size)
		puts("[wave] warning: incomplete read!");

	sf_close(fileIn);
	return 1;
}





int Wave::writeData(const char *f) {

	

	outHeader.samplerate = inHeader.samplerate;
	outHeader.channels   = inHeader.channels;
	outHeader.format     = inHeader.format;

	fileOut = sf_open(f, SFM_WRITE, &outHeader);
	if (fileOut == NULL) {
		printf("[wave] unable to open %s for exporting\n", f);
		return 0;
	}

	int out = sf_write_float(fileOut, data, size);
	if (out != (int) size) {
		printf("[wave] error while exporting %s! %s\n", f, sf_strerror(fileOut));
		return 0;
	}

	isLogical = false;
	isEdited  = false;
	sf_close(fileOut);
	return 1;
}





void Wave::clear() {
	if (data != NULL) {
		free(data);
		data     = NULL;
		pathfile = "";
		size     = 0;
	}
}





int Wave::allocEmpty(unsigned __size) {

	

	
	size = __size;
	data = (float *) malloc(size * sizeof(float));
	if (data == NULL) {
		puts("[wave] unable to allocate memory");
		return 0;
	}

	memset(data, 0, sizeof(float) * size); 

	inHeader.samplerate = G_Conf.samplerate;
	inHeader.channels   = 2;
	inHeader.format     = SF_FORMAT_WAV | SF_FORMAT_FLOAT; 

	isLogical = true;
	return 1;
}





int Wave::resample(int quality, int newRate) {

	float ratio = newRate / (float) inHeader.samplerate;
	int newSize = ceil(size * ratio);
	if (newSize % 2 != 0)   
		newSize++;

	float *tmp = (float *) malloc(newSize * sizeof(float));
	if (!tmp) {
		puts("[wave] unable to allocate memory for resampling");
		return -1;
	}

	SRC_DATA src_data;
	src_data.data_in       = data;
	src_data.input_frames  = size/2;     
	src_data.data_out      = tmp;
	src_data.output_frames = newSize/2;  
	src_data.src_ratio     = ratio;

	printf("[wave] resampling: new size=%d (%d frames)\n", newSize, newSize/2);

	int ret = src_simple(&src_data, quality, 2);
	if (ret != 0) {
		printf("[wave] resampling error: %s\n", src_strerror(ret));
		return 0;
	}

	free(data);
	data = tmp;
	size = newSize;
	inHeader.samplerate = newRate;
	return 1;
}





std::string Wave::basename() {
	return gStripExt(gBasename(pathfile.c_str()).c_str());
}

std::string Wave::extension() {
	return gGetExt(pathfile.c_str());
}





void Wave::updateName(const char *n) {

	std::string ext = gGetExt(pathfile.c_str());

	name     = gStripExt(gBasename(n).c_str());
	pathfile = gDirname(pathfile.c_str()) + gGetSlash() + name + "." + ext;
}
/* ---------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ---------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------ */




#include <math.h>
#include "waveFx.h"
#include "channel.h"
#include "mixer.h"
#include "wave.h"


extern Mixer G_Mixer;


float wfx_normalizeSoft(Wave *w) {
	float peak = 0.0f;
	float abs  = 0.0f;
	for (int i=0; i<w->size; i++) { 
		abs = fabs(w->data[i]);
		if (abs > peak)
			peak = abs;
	}

	

	if (peak == 0.0f || peak > 1.0f)
		return 1.0f;

	return 1.0f / peak;
}





bool wfx_monoToStereo(Wave *w) {

	unsigned newSize = w->size * 2;
	float *dataNew = (float *) malloc(newSize * sizeof(float));
	if (dataNew == NULL) {
		printf("[wfx] unable to allocate memory for mono>stereo conversion\n");
		return 0;
	}

	for (int i=0, j=0; i<w->size; i++) {
		dataNew[j]   = w->data[i];
		dataNew[j+1] = w->data[i];
		j+=2;
	}

	free(w->data);
	w->data = dataNew;
	w->size = newSize;
	w->frames(w->frames()*2);
	w->channels(2);

	return 1;
}





void wfx_silence(Wave *w, int a, int b) {

	
	a = a * 2;
	b = b * 2;

	printf("[wfx] silencing from %d to %d\n", a, b);

	for (int i=a; i<b; i+=2) {
		w->data[i]   = 0.0f;
		w->data[i+1] = 0.0f;
	}

	w->isEdited = true;

	return;
}





int wfx_cut(Wave *w, int a, int b) {
	a = a * 2;
	b = b * 2;

	if (a < 0) a = 0;
	if (b > w->size) b = w->size;

	

	unsigned newSize = w->size-(b-a);
	float *temp = (float *) malloc(newSize * sizeof(float));
	if (temp == NULL) {
		puts("[wfx] unable to allocate memory for cutting");
		return 0;
	}

	printf("[wfx] cutting from %d to %d, new size=%d (video=%d)\n", a, b, newSize, newSize/2);

	for (int i=0, k=0; i<w->size; i++) {
		if (i < a || i >= b) {		               
			temp[k] = w->data[i];   
			k++;
		}
	}

	free(w->data);
	w->data = temp;
	w->size = newSize;
	
	w->frames(w->frames() - b - a);
	w->isEdited = true;

	puts("[wfx] cutting done");

	return 1;
}





int wfx_trim(Wave *w, int a, int b) {
	a = a * 2;
	b = b * 2;

	if (a < 0) a = 0;
	if (b > w->size) b = w->size;

	int newSize = b - a;
	float *temp = (float *) malloc(newSize * sizeof(float));
	if (temp == NULL) {
		puts("[wfx] unable to allocate memory for trimming");
		return 0;
	}

	printf("[wfx] trimming from %d to %d (area = %d)\n", a, b, b-a);

	for (int i=a, k=0; i<b; i++, k++)
		temp[k] = w->data[i];

	free(w->data);
	w->data = temp;
	w->size = newSize;
	
	w->frames(b - a);
 	w->isEdited = true;

	return 1;
}





void wfx_fade(Wave *w, int a, int b, int type) {

	float m = type == 0 ? 0.0f : 1.0f;
	float d = 1.0f/(float)(b-a);
	if (type == 1)
		d = -d;

	a *= 2;
	b *= 2;

	for (int i=a; i<b; i+=2) {
		w->data[i]   *= m;
		w->data[i+1] *= m;
		m += d;
	}
}





void wfx_smooth(Wave *w, int a, int b) {

	int d = 32;  

	

	if (d*2 > (b-a)*2) {
		puts("[WFX] selection is too small, nothing to do");
		return;
	}

	wfx_fade(w, a, a+d, 0);
	wfx_fade(w, b-d, b, 1);
}
