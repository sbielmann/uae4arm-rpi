 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Config file handling
  * This still needs some thought before it's complete...
  *
  * Copyright 1998 Brian King, Bernd Schmidt
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include <ctype.h>

#include "options.h"
#include "uae.h"
#include "audio.h"
#include "autoconf.h"
#include "inputdevice.h"
#include "savestate.h"
#include "gui.h"
#include "memory.h"
#include "rommgr.h"
#include "newcpu.h"
#include "custom.h"
#include "zfile.h"
#include "filesys.h"
#include "fsdb.h"
#include "disk.h"
#include "blkdev.h"
#include "sd-pandora/sound.h"

#include "SDL_keysym.h"

static int config_newfilesystem;
static struct strlist *temp_lines;
static struct zfile *default_file;
static int uaeconfig;

/* @@@ need to get rid of this... just cut part of the manual and print that
 * as a help text.  */
struct cfg_lines
{
  const TCHAR *config_label, *config_help;
};

static const TCHAR *guimode1[] = { _T("no"), _T("yes"), _T("nowait"), 0 };
static const TCHAR *guimode2[] = { _T("false"), _T("true"), _T("nowait"), 0 };
static const TCHAR *guimode3[] = { _T("0"), _T("1"), _T("nowait"), 0 };
static const TCHAR *csmode[] = { _T("ocs"), _T("ecs_agnus"), _T("ecs_denise"), _T("ecs"), _T("aga"), 0 };
static const TCHAR *speedmode[] = { _T("max"), _T("real"), 0 };
static const TCHAR *soundmode1[] = { _T("none"), _T("interrupts"), _T("normal"), _T("exact"), 0 };
static const TCHAR *soundmode2[] = { _T("none"), _T("interrupts"), _T("good"), _T("best"), 0 };
static const TCHAR *stereomode[] = { _T("mono"), _T("stereo"), _T("clonedstereo"), _T("4ch"), _T("clonedstereo6ch"), _T("6ch"), _T("mixed"), 0 };
static const TCHAR *interpolmode[] = { _T("none"), _T("anti"), _T("sinc"), _T("rh"), _T("crux"), 0 };
static const TCHAR *collmode[] = { _T("none"), _T("sprites"), _T("playfields"), _T("full"), 0 };
static const TCHAR *soundfiltermode1[] = { _T("off"), _T("emulated"), _T("on"), 0 };
static const TCHAR *soundfiltermode2[] = { _T("standard"), _T("enhanced"), 0 };
static const TCHAR *lorestype1[] = { _T("lores"), _T("hires"), _T("superhires"), 0 };
static const TCHAR *lorestype2[] = { _T("true"), _T("false"), 0 };
static const TCHAR *abspointers[] = { _T("none"), _T("mousehack"), _T("tablet"), 0 };
static const TCHAR *joyportmodes[] = { _T(""), _T("mouse"), _T("djoy"), _T("gamepad"), _T("ajoy"), _T("cdtvjoy"), _T("cd32joy"), _T("lightpen"), 0 };
static const TCHAR *joyaf[] = { _T("none"), _T("normal"), _T("toggle"), 0 };
static const TCHAR *cdmodes[] = { _T("disabled"), _T(""), _T("image"), _T("ioctl"), _T("spti"), _T("aspi"), 0 };
static const TCHAR *cdconmodes[] = { _T(""), _T("uae"), _T("ide"), _T("scsi"), _T("cdtv"), _T("cd32"), 0 };
static const TCHAR *rtgtype[] = { _T("ZorroII"), _T("ZorroIII"), 0 };

static const TCHAR *obsolete[] = {
	_T("accuracy"), _T("gfx_opengl"), _T("gfx_32bit_blits"), _T("32bit_blits"),
	_T("gfx_immediate_blits"), _T("gfx_ntsc"), _T("win32"), _T("gfx_filter_bits"),
	_T("sound_pri_cutoff"), _T("sound_pri_time"), _T("sound_min_buff"), _T("sound_bits"),
	_T("gfx_test_speed"), _T("gfxlib_replacement"), _T("enforcer"), _T("catweasel_io"),
  _T("kickstart_key_file"), _T("sound_adjust"), _T("sound_latency"),
	_T("serial_hardware_dtrdsr"), _T("gfx_filter_upscale"),
	_T("gfx_autoscale"), _T("parallel_sampler"), _T("parallel_ascii_emulation"),
	_T("avoid_vid"), _T("avoid_dga"), _T("z3chipmem_size"), _T("state_replay_buffer"), _T("state_replay"),
    NULL
};

#define UNEXPANDED _T("$(FILE_PATH)")

static void trimwsa (char *s)
{
  /* Delete trailing whitespace.  */
  int len = strlen (s);
  while (len > 0 && strcspn (s + len - 1, "\t \r\n") == 0)
    s[--len] = '\0';
}

static int match_string (const TCHAR *table[], const TCHAR *str)
{
  int i;
  for (i = 0; table[i] != 0; i++)
  	if (strcasecmp (table[i], str) == 0)
	    return i;
  return -1;
}

static TCHAR *cfgfile_subst_path2 (const TCHAR *path, const TCHAR *subst, const TCHAR *file)
{
  /* @@@ use strcasecmp for some targets.  */
  if (_tcslen (path) > 0 && _tcsncmp (file, path, _tcslen (path)) == 0) {
  	int l;
  	TCHAR *p2, *p = xmalloc (TCHAR, _tcslen (file) + _tcslen (subst) + 2);
  	_tcscpy (p, subst);
  	l = _tcslen (p);
  	while (l > 0 && p[l - 1] == '/')
	    p[--l] = '\0';
  	l = _tcslen (path);
  	while (file[l] == '/')
	    l++;
		_tcscat (p, _T("/"));
    _tcscat (p, file + l);
  	p2 = target_expand_environment (p);
		xfree (p);
	  return p2;
  }
	return NULL;
}

TCHAR *cfgfile_subst_path (const TCHAR *path, const TCHAR *subst, const TCHAR *file)
{
	TCHAR *s = cfgfile_subst_path2 (path, subst, file);
	if (s)
		return s;
	s = target_expand_environment (file);
	return s;
}

static bool isdefault (const TCHAR *s)
{
  TCHAR tmp[MAX_DPATH];
  if (!default_file || uaeconfig)
  	return false;
  zfile_fseek (default_file, 0, SEEK_SET);
  while (zfile_fgets (tmp, sizeof tmp / sizeof (TCHAR), default_file)) {
  	if (tmp[0] && tmp[_tcslen (tmp) - 1] == '\n')
  		tmp[_tcslen (tmp) - 1] = 0;
  	if (!_tcscmp (tmp, s))
	    return true;
  }
  return false;
}

static size_t cfg_write (const void *b, struct zfile *z)
{
  size_t v;
  TCHAR lf = 10;
  v = zfile_fwrite ((void *)b, _tcslen ((TCHAR*)b), sizeof (TCHAR), z);
  zfile_fwrite (&lf, 1, 1, z);
  return v;
}

static void cfg_dowrite (struct zfile *f, const TCHAR *option, const TCHAR *value, int d, int target)
{
  TCHAR tmp[CONFIG_BLEN];

	if (value == NULL)
		return;

  if (target)
  	_stprintf (tmp, _T("%s.%s=%s"), TARGET_NAME, option, value);
  else
  	_stprintf (tmp, _T("%s=%s"), option, value);
  if (d && isdefault (tmp))
  	goto end;
  cfg_write (tmp, f);
end:;
}

void cfgfile_write_bool (struct zfile *f, const TCHAR *option, bool b)
{
	cfg_dowrite (f, option, b ? _T("true") : _T("false"), 0, 0);
}
void cfgfile_dwrite_bool (struct zfile *f, const TCHAR *option, bool b)
{
	cfg_dowrite (f, option, b ? _T("true") : _T("false"), 1, 0);
}
void cfgfile_dwrite_bool (struct zfile *f, const TCHAR *option, int b)
{
	cfgfile_dwrite_bool (f, option, b != 0);
}
void cfgfile_write_str (struct zfile *f, const TCHAR *option, const TCHAR *value)
{
  cfg_dowrite (f, option, value, 0, 0);
}
void cfgfile_dwrite_str (struct zfile *f, const TCHAR *option, const TCHAR *value)
{
  cfg_dowrite (f, option, value, 1, 0);
}

void cfgfile_target_write_bool (struct zfile *f, const TCHAR *option, bool b)
{
	cfg_dowrite (f, option, b ? _T("true") : _T("false"), 0, 1);
}
void cfgfile_target_dwrite_bool (struct zfile *f, const TCHAR *option, bool b)
{
	cfg_dowrite (f, option, b ? _T("true") : _T("false"), 1, 1);
}
void cfgfile_target_write_str (struct zfile *f, const TCHAR *option, const TCHAR *value)
{
  cfg_dowrite (f, option, value, 0, 1);
}
void cfgfile_target_dwrite_str (struct zfile *f, const TCHAR *option, const TCHAR *value)
{
  cfg_dowrite (f, option, value, 1, 1);
}

void cfgfile_write (struct zfile *f, const TCHAR *option, const TCHAR *format,...)
{
  va_list parms;
  TCHAR tmp[CONFIG_BLEN];

  va_start (parms, format);
	_vsntprintf (tmp, CONFIG_BLEN, format, parms);
  cfg_dowrite (f, option, tmp, 0, 0);
  va_end (parms);
}
void cfgfile_dwrite (struct zfile *f, const TCHAR *option, const TCHAR *format,...)
{
  va_list parms;
  TCHAR tmp[CONFIG_BLEN];

  va_start (parms, format);
	_vsntprintf (tmp, CONFIG_BLEN, format, parms);
  cfg_dowrite (f, option, tmp, 1, 0);
  va_end (parms);
}
void cfgfile_target_write (struct zfile *f, const TCHAR *option, const TCHAR *format,...)
{
  va_list parms;
  TCHAR tmp[CONFIG_BLEN];

  va_start (parms, format);
	_vsntprintf (tmp, CONFIG_BLEN, format, parms);
  cfg_dowrite (f, option, tmp, 0, 1);
  va_end (parms);
}
void cfgfile_target_dwrite (struct zfile *f, const TCHAR *option, const TCHAR *format,...)
{
  va_list parms;
  TCHAR tmp[CONFIG_BLEN];

  va_start (parms, format);
	_vsntprintf (tmp, CONFIG_BLEN, format, parms);
  cfg_dowrite (f, option, tmp, 1, 1);
  va_end (parms);
}

static void cfgfile_write_rom (struct zfile *f, const TCHAR *path, const TCHAR *romfile, const TCHAR *name)
{
	TCHAR *str = cfgfile_subst_path (path, UNEXPANDED, romfile);
	cfgfile_write_str (f, name, str);
	struct zfile *zf = zfile_fopen (str, _T("rb"), ZFD_ALL);
	if (zf) {
		struct romdata *rd = getromdatabyzfile (zf);
		if (rd) {
			TCHAR name2[MAX_DPATH], str2[MAX_DPATH];
			_tcscpy (name2, name);
			_tcscat (name2, _T("_id"));
			_stprintf (str2, _T("%08X,%s"), rd->crc32, rd->name);
			cfgfile_write_str (f, name2, str2);
		}
		zfile_fclose (zf);
	}
	xfree (str);

}

static void write_filesys_config (struct uae_prefs *p, const TCHAR *unexpanded,
				  const TCHAR *default_path, struct zfile *f)
{
  int i;
  TCHAR tmp[MAX_DPATH], tmp2[MAX_DPATH];
  const TCHAR *hdcontrollers[] = { _T("uae"), 
    _T("ide0"), _T("ide1"), _T("ide2"), _T("ide3"),
	  _T("scsi0"), _T("scsi1"), _T("scsi2"), _T("scsi3"), _T("scsi4"), _T("scsi5"), _T("scsi6"),
	  _T("scsram"), _T("scide") }; /* scsram = smart card sram = pcmcia sram card */

  for (i = 0; i < p->mountitems; i++) {
	  struct uaedev_config_info *uci = &p->mountconfig[i];
	  TCHAR *str;
    int bp = uci->bootpri;

  	if (!uci->autoboot)
	    bp = -128;
  	if (uci->donotmount)
	    bp = -129;
  	str = cfgfile_subst_path (default_path, unexpanded, uci->rootdir);
  	if (!uci->ishdf) {
	    _stprintf (tmp, _T("%s,%s:%s:%s,%d"), uci->readonly ? _T("ro") : _T("rw"),
  		  uci->devname ? uci->devname : _T(""), uci->volname, str, bp);
	    cfgfile_write_str (f, _T("filesystem2"), tmp);
	  } else {
	    _stprintf (tmp, _T("%s,%s:%s,%d,%d,%d,%d,%d,%s,%s"),
		    uci->readonly ? _T("ro") : _T("rw"),
		    uci->devname ? uci->devname : _T(""), str,
		    uci->sectors, uci->surfaces, uci->reserved, uci->blocksize,
		    bp, uci->filesys ? uci->filesys : _T(""), hdcontrollers[uci->controller]);
	    cfgfile_write_str (f, _T("hardfile2"), tmp);
	  }
	  _stprintf (tmp2, _T("uaehf%d"), i);
    cfgfile_write (f, tmp2, _T("%s,%s"), uci->ishdf ? _T("hdf") : _T("dir"), tmp);
	  xfree (str);
  }
}

static void write_compatibility_cpu(struct zfile *f, struct uae_prefs *p)
{
  TCHAR tmp[100];
  int model;

  model = p->cpu_model;
  if (model == 68030)
  	model = 68020;
  if (model == 68060)
  	model = 68040;
  if (p->address_space_24 && model == 68020)
  	_tcscpy (tmp, _T("68ec020"));
  else
	  _stprintf(tmp, _T("%d"), model);
  if (model == 68020 && (p->fpu_model == 68881 || p->fpu_model == 68882)) 
  	_tcscat(tmp, _T("/68881"));
  cfgfile_write (f, _T("cpu_type"), tmp);
}

void cfgfile_save_options (struct zfile *f, struct uae_prefs *p, int type)
{
  struct strlist *sl;
  TCHAR *str, tmp[MAX_DPATH];
  int i;

  cfgfile_write_str (f, _T("config_description"), p->description);
  cfgfile_write_bool (f, _T("config_hardware"), type & CONFIG_TYPE_HARDWARE);
  cfgfile_write_bool (f, _T("config_host"), !!(type & CONFIG_TYPE_HOST));
  if (p->info[0])
  	cfgfile_write (f, _T("config_info"), p->info);
  cfgfile_write (f, _T("config_version"), _T("%d.%d.%d"), UAEMAJOR, UAEMINOR, UAESUBREV);
   
  for (sl = p->all_lines; sl; sl = sl->next) {
  	if (sl->unknown) {
			if (sl->option)
	      cfgfile_write_str (f, sl->option, sl->value);
    }
  }

  _stprintf (tmp, _T("%s.rom_path"), TARGET_NAME);
  cfgfile_write_str (f, tmp, p->path_rom);
  _stprintf (tmp, _T("%s.floppy_path"), TARGET_NAME);
  cfgfile_write_str (f, tmp, p->path_floppy);
  _stprintf (tmp, _T("%s.hardfile_path"), TARGET_NAME);
  cfgfile_write_str (f, tmp, p->path_hardfile);
	_stprintf (tmp, _T("%s.cd_path"), TARGET_NAME);
	cfgfile_write_str (f, tmp, p->path_cd);

  cfg_write (_T("; host-specific"), f);

  target_save_options (f, p);

  cfg_write (_T("; common"), f);

  cfgfile_write_str (f, _T("use_gui"), guimode1[p->start_gui]);
	cfgfile_write_rom (f, p->path_rom, p->romfile, _T("kickstart_rom_file"));
  cfgfile_write_rom (f, p->path_rom, p->romextfile, _T("kickstart_ext_rom_file"));
	cfgfile_write_str (f, _T("flash_file"), p->flashfile);

  p->nr_floppies = 4;
  for (i = 0; i < 4; i++) {
  	str = cfgfile_subst_path (p->path_floppy, UNEXPANDED, p->floppyslots[i].df);
    _stprintf (tmp, _T("floppy%d"), i);
  	cfgfile_write_str (f, tmp, str);
  	xfree (str);
  	_stprintf (tmp, _T("floppy%dtype"), i);
  	cfgfile_dwrite (f, tmp, _T("%d"), p->floppyslots[i].dfxtype);
  	if (p->floppyslots[i].dfxtype < 0 && p->nr_floppies > i)
	    p->nr_floppies = i;
  }

	for (i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		if (p->cdslots[i].name[0] || p->cdslots[i].inuse) {
			TCHAR tmp2[MAX_DPATH];
			_stprintf (tmp, _T("cdimage%d"), i);
			TCHAR *s = cfgfile_subst_path (p->path_cd, UNEXPANDED, p->cdslots[i].name);
			_tcscpy (tmp2, s);
			xfree (s);
			if (p->cdslots[i].type != SCSI_UNIT_DEFAULT || _tcschr (p->cdslots[i].name, ',') || p->cdslots[i].delayed) {
				_tcscat (tmp2, _T(","));
				if (p->cdslots[i].delayed) {
					_tcscat (tmp2, _T("delay"));
					_tcscat (tmp2, _T(":"));
				}
				if (p->cdslots[i].type != SCSI_UNIT_DEFAULT) {
					_tcscat (tmp2, cdmodes[p->cdslots[i].type + 1]);
				}
			}
			cfgfile_write_str (f, tmp, tmp2);
		}
	}

  cfgfile_write (f, _T("nr_floppies"), _T("%d"), p->nr_floppies);
  cfgfile_write (f, _T("floppy_speed"), _T("%d"), p->floppy_speed);

  cfgfile_write_str (f, _T("sound_output"), soundmode1[p->produce_sound]);
  cfgfile_write_str (f, _T("sound_channels"), stereomode[p->sound_stereo]);
  cfgfile_write (f, _T("sound_stereo_separation"), _T("%d"), p->sound_stereo_separation);
  cfgfile_write (f, _T("sound_stereo_mixing_delay"), _T("%d"), p->sound_mixed_stereo_delay >= 0 ? p->sound_mixed_stereo_delay : 0);
  cfgfile_write (f, _T("sound_frequency"), _T("%d"), p->sound_freq);
  cfgfile_write_str (f, _T("sound_interpol"), interpolmode[p->sound_interpol]);
  cfgfile_write_str (f, _T("sound_filter"), soundfiltermode1[p->sound_filter]);
  cfgfile_write_str (f, _T("sound_filter_type"), soundfiltermode2[p->sound_filter_type]);
	if (p->sound_volume_cd >= 0)
		cfgfile_write (f, _T("sound_volume_cd"), _T("%d"), p->sound_volume_cd);

  cfgfile_write (f, _T("cachesize"), _T("%d"), p->cachesize);

	for (i = 0; i < MAX_JPORTS; i++) {
		struct jport *jp = &p->jports[i];
		int v = jp->id;
		TCHAR tmp1[MAX_DPATH], tmp2[MAX_DPATH];
		if (v == JPORT_CUSTOM) {
			_tcscpy (tmp2, _T("custom"));
		} else if (v == JPORT_NONE) {
			_tcscpy (tmp2, _T("none"));
		} else if (v < JSEM_JOYS) {
			_stprintf (tmp2, _T("kbd%d"), v + 1);
		} else if (v < JSEM_MICE) {
			_stprintf (tmp2, _T("joy%d"), v - JSEM_JOYS);
		} else {
			_tcscpy (tmp2, _T("mouse"));
			if (v - JSEM_MICE > 0)
				_stprintf (tmp2, _T("mouse%d"), v - JSEM_MICE);
		}
		if (i < 2 || jp->id >= 0) {
			_stprintf (tmp1, _T("joyport%d"), i);
			cfgfile_write (f, tmp1, tmp2);
			_stprintf (tmp1, _T("joyport%dautofire"), i);
			cfgfile_write (f, tmp1, joyaf[jp->autofire]);
			if (i < 2 && jp->mode > 0) {
				_stprintf (tmp1, _T("joyport%dmode"), i);
				cfgfile_write (f, tmp1, joyportmodes[jp->mode]);
			}
			if (jp->name[0]) {
				_stprintf (tmp1, _T("joyportfriendlyname%d"), i);
				cfgfile_write (f, tmp1, jp->name);
			}
			if (jp->configname[0]) {
				_stprintf (tmp1, _T("joyportname%d"), i);
				cfgfile_write (f, tmp1, jp->configname);
			}
		}
	}

	cfgfile_write_bool (f, _T("bsdsocket_emu"), p->socket_emu);

  cfgfile_write_bool (f, _T("synchronize_clock"), p->tod_hack);
  cfgfile_dwrite_str (f, _T("absolute_mouse"), abspointers[p->input_tablet]);

  cfgfile_write (f, _T("key_for_menu"), _T("%d"), p->key_for_menu);

  cfgfile_write (f, _T("gfx_framerate"), _T("%d"), p->gfx_framerate);
  cfgfile_write (f, _T("gfx_width"), _T("%d"), p->gfx_size.width);
  cfgfile_write (f, _T("gfx_height"), _T("%d"), p->gfx_size.height);
  cfgfile_write (f, _T("gfx_width_windowed"), _T("%d"), p->gfx_size_win.width);
  cfgfile_write (f, _T("gfx_height_windowed"), _T("%d"), p->gfx_size_win.height);
  cfgfile_write (f, _T("gfx_width_fullscreen"), _T("%d"), p->gfx_size_fs.width);
  cfgfile_write (f, _T("gfx_height_fullscreen"), _T("%d"), p->gfx_size_fs.height);
  cfgfile_write_bool (f, _T("gfx_lores"), p->gfx_resolution == 0);
  cfgfile_write_str (f, _T("gfx_resolution"), lorestype1[p->gfx_resolution]);

#ifdef RASPBERRY
  cfgfile_write (f, _T("gfx_correct_aspect"), _T("%d"), p->gfx_correct_aspect);
  cfgfile_write (f, _T("gfx_fullscreen_ratio"), _T("%d"), p->gfx_fullscreen_ratio);
#endif

  cfgfile_write_bool (f, _T("immediate_blits"), p->immediate_blits);
  cfgfile_write_bool (f, _T("fast_copper"), p->fast_copper);
  cfgfile_write_bool (f, _T("ntsc"), p->ntscmode);
  cfgfile_dwrite_bool (f, _T("show_leds"), p->leds_on_screen);
  if (p->chipset_mask & CSMASK_AGA)
  	cfgfile_dwrite (f, _T("chipset"), _T("aga"));
  else if ((p->chipset_mask & CSMASK_ECS_AGNUS) && (p->chipset_mask & CSMASK_ECS_DENISE))
  	cfgfile_dwrite (f, _T("chipset"), _T("ecs"));
  else if (p->chipset_mask & CSMASK_ECS_AGNUS)
  	cfgfile_dwrite (f, _T("chipset"), _T("ecs_agnus"));
  else if (p->chipset_mask & CSMASK_ECS_DENISE)
  	cfgfile_dwrite (f, _T("chipset"), _T("ecs_denise"));
  else
  	cfgfile_dwrite (f, _T("chipset"), _T("ocs"));
  cfgfile_write_str (f, _T("collision_level"), collmode[p->collision_level]);

	cfgfile_dwrite_bool (f, _T("cd32cd"), p->cs_cd32cd);
	cfgfile_dwrite_bool (f, _T("cd32c2p"), p->cs_cd32c2p);
	cfgfile_dwrite_bool (f, _T("cd32nvram"), p->cs_cd32nvram);

  cfgfile_write (f, _T("fastmem_size"), _T("%d"), p->fastmem_size / 0x100000);
  cfgfile_write (f, _T("z3mem_size"), _T("%d"), p->z3fastmem_size / 0x100000);
  cfgfile_write (f, _T("z3mem_start"), _T("0x%x"), p->z3fastmem_start);
  cfgfile_write (f, _T("bogomem_size"), _T("%d"), p->bogomem_size / 0x40000);
  cfgfile_write (f, _T("gfxcard_size"), _T("%d"), p->rtgmem_size / 0x100000);
	cfgfile_write_str (f, _T("gfxcard_type"), rtgtype[p->rtgmem_type]);
  cfgfile_write (f, _T("chipmem_size"), _T("%d"), p->chipmem_size == 0x20000 ? -1 : (p->chipmem_size == 0x40000 ? 0 : p->chipmem_size / 0x80000));

  if (p->m68k_speed > 0) {
  	cfgfile_write (f, _T("finegrain_cpu_speed"), _T("%d"), p->m68k_speed);
	} else {
  	cfgfile_write_str (f, _T("cpu_speed"), p->m68k_speed < 0 ? _T("max") : _T("real"));
  }

  /* do not reorder start */
  write_compatibility_cpu(f, p);
  cfgfile_write (f, _T("cpu_model"), _T("%d"), p->cpu_model);
  if (p->fpu_model)
  	cfgfile_write (f, _T("fpu_model"), _T("%d"), p->fpu_model);
  cfgfile_write_bool (f, _T("cpu_compatible"), p->cpu_compatible);
  cfgfile_write_bool (f, _T("cpu_24bit_addressing"), p->address_space_24);
  /* do not reorder end */
  cfgfile_write (f, _T("rtg_modes"), _T("0x%x"), p->picasso96_modeflags);

#ifdef FILESYS
  write_filesys_config (p, UNEXPANDED, p->path_hardfile, f);
#endif
  write_inputdevice_config (p, f);
}

int cfgfile_yesno (const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location)
{
  if (name != NULL && _tcscmp (option, name) != 0)
  	return 0;
	if (strcasecmp (value, _T("yes")) == 0 || strcasecmp (value, _T("y")) == 0
		|| strcasecmp (value, _T("true")) == 0 || strcasecmp (value, _T("t")) == 0)
  	*location = 1;
	else if (strcasecmp (value, _T("no")) == 0 || strcasecmp (value, _T("n")) == 0
		|| strcasecmp (value, _T("false")) == 0 || strcasecmp (value, _T("f")) == 0
		|| strcasecmp (value, _T("0")) == 0)
	  *location = 0;
  else {
	  write_log (_T("Option `%s' requires a value of either `yes' or `no' (was '%s').\n"), option, value);
	  return -1;
  }
  return 1;
}

int cfgfile_yesno (const TCHAR *option, const TCHAR *value, const TCHAR *name, bool *location)
{
	int val;
	int ret = cfgfile_yesno (option, value, name, &val);
	if (ret == 0)
		return 0;
	if (ret < 0)
		*location = false;
	else
		*location = val != 0;
	return 1;
}

int cfgfile_doubleval (const TCHAR *option, const TCHAR *value, const TCHAR *name, double *location)
{
	int base = 10;
	TCHAR *endptr;
	if (name != NULL && _tcscmp (option, name) != 0)
		return 0;
	*location = _tcstod (value, &endptr);
	return 1;
}

int cfgfile_intval (const TCHAR *option, const TCHAR *value, const TCHAR *name, unsigned int *location, int scale)
{
  int base = 10;
  TCHAR *endptr;
  if (name != NULL && _tcscmp (option, name) != 0)
  	return 0;
  /* I guess octal isn't popular enough to worry about here...  */
  if (value[0] == '0' && _totupper(value[1]) == 'X')
  	value += 2, base = 16;
  *location = _tcstol (value, &endptr, base) * scale;

  if (*endptr != '\0' || *value == '\0') {
		if (strcasecmp (value, _T("false")) == 0 || strcasecmp (value, _T("no")) == 0) {
			*location = 0;
			return 1;
		}
		if (strcasecmp (value, _T("true")) == 0 || strcasecmp (value, _T("yes")) == 0) {
			*location = 1;
			return 1;
  	}
		write_log (_T("Option '%s' requires a numeric argument but got '%s'\n"), option, value);
  	return -1;
  }
  return 1;
}
int cfgfile_intval (const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location, int scale)
{
	unsigned int v = 0;
	int r = cfgfile_intval (option, value, name, &v, scale);
	if (!r)
		return 0;
	*location = (int)v;
	return r;
}

int cfgfile_strval (const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location, const TCHAR *table[], int more)
{
  int val;
  if (name != NULL && _tcscmp (option, name) != 0)
  	return 0;
  val = match_string (table, value);
  if (val == -1) {
  	if (more)
	    return 0;

		write_log (_T("Unknown value ('%s') for option '%s'.\n"), value, option);
  	return -1;
  }
  *location = val;
  return 1;
}

int cfgfile_strboolval (const TCHAR *option, const TCHAR *value, const TCHAR *name, bool *location, const TCHAR *table[], int more)
{
	int locationint;
	if (!cfgfile_strval (option, value, name, &locationint, table, more))
		return 0;
	*location = locationint != 0;
	return 1;
}

int cfgfile_string (const TCHAR *option, const TCHAR *value, const TCHAR *name, TCHAR *location, int maxsz)
{
  if (_tcscmp (option, name) != 0)
  	return 0;
  _tcsncpy (location, value, maxsz - 1);
  location[maxsz - 1] = '\0';
  return 1;
}

int cfgfile_path (const TCHAR *option, const TCHAR *value, const TCHAR *name, TCHAR *location, int maxsz)
{
	if (!cfgfile_string (option, value, name, location, maxsz))
		return 0;
	TCHAR *s = target_expand_environment (location);
	_tcsncpy (location, s, maxsz - 1);
	location[maxsz - 1] = 0;
	xfree (s);
	return 1;
}

int cfgfile_rom (const TCHAR *option, const TCHAR *value, const TCHAR *name, TCHAR *location, int maxsz)
{
	TCHAR id[MAX_DPATH];
	if (!cfgfile_string (option, value, name, id, sizeof id / sizeof (TCHAR)))
		return 0;
	if (zfile_exists (location))
		return 1;
	TCHAR *p = _tcschr (id, ',');
	if (p) {
		TCHAR *endptr, tmp;
		*p = 0;
		tmp = id[4];
		id[4] = 0;
		uae_u32 crc32 = _tcstol (id, &endptr, 16) << 16;
		id[4] = tmp;
		crc32 |= _tcstol (id + 4, &endptr, 16);
		struct romdata *rd = getromdatabycrc (crc32);
		if (rd) {
			struct romlist *rl = getromlistbyromdata (rd);
			if (rl) {
				write_log (_T("%s: %s -> %s\n"), name, location, rl->path);
				_tcsncpy (location, rl->path, maxsz);
			}
		}
	}
	return 1;
}

static int getintval (TCHAR **p, int *result, int delim)
{
  TCHAR *value = *p;
  int base = 10;
  TCHAR *endptr;
  TCHAR *p2 = _tcschr (*p, delim);

  if (p2 == 0)
  	return 0;

  *p2++ = '\0';

  if (value[0] == '0' && _totupper (value[1]) == 'X')
  	value += 2, base = 16;
  *result = _tcstol (value, &endptr, base);
  *p = p2;

  if (*endptr != '\0' || *value == '\0')
  	return 0;

  return 1;
}

static int getintval2 (TCHAR **p, int *result, int delim)
{
  TCHAR *value = *p;
  int base = 10;
  TCHAR *endptr;
  TCHAR *p2 = _tcschr (*p, delim);

  if (p2 == 0) {
	  p2 = _tcschr (*p, 0);
	  if (p2 == 0) {
	    *p = 0;
	    return 0;
  	}
  }
  if (*p2 != 0)
  	*p2++ = '\0';

  if (value[0] == '0' && _totupper (value[1]) == 'X')
  	value += 2, base = 16;
  *result = _tcstol (value, &endptr, base);
  *p = p2;

  if (*endptr != '\0' || *value == '\0') {
	  *p = 0;
	  return 0;
  }

  return 1;
}

static void set_chipset_mask (struct uae_prefs *p, int val)
{
  p->chipset_mask = (val == 0 ? 0
    : val == 1 ? CSMASK_ECS_AGNUS
    : val == 2 ? CSMASK_ECS_DENISE
    : val == 3 ? CSMASK_ECS_DENISE | CSMASK_ECS_AGNUS
    : CSMASK_AGA | CSMASK_ECS_DENISE | CSMASK_ECS_AGNUS);
}

static int cfgfile_parse_host (struct uae_prefs *p, TCHAR *option, TCHAR *value)
{
  int i;
  bool vb;
  TCHAR *section = 0;
  TCHAR *tmpp;
  TCHAR tmpbuf[CONFIG_BLEN];

	if (_tcsncmp (option, _T("input."), 6) == 0) {
    read_inputdevice_config (p, option, value);
    return 1;
  }

  for (tmpp = option; *tmpp != '\0'; tmpp++)
  	if (_istupper (*tmpp))
	    *tmpp = _totlower (*tmpp);
  tmpp = _tcschr (option, '.');
  if (tmpp) {
    section = option;
    option = tmpp + 1;
    *tmpp = '\0';
    if (_tcscmp (section, TARGET_NAME) == 0) {
	    /* We special case the various path options here.  */
      if (cfgfile_path (option, value, _T("rom_path"), p->path_rom, sizeof p->path_rom / sizeof (TCHAR))
  		|| cfgfile_path (option, value, _T("floppy_path"), p->path_floppy, sizeof p->path_floppy / sizeof (TCHAR))
  		|| cfgfile_path (option, value, _T("cd_path"), p->path_cd, sizeof p->path_cd / sizeof (TCHAR))
  		|| cfgfile_path (option, value, _T("hardfile_path"), p->path_hardfile, sizeof p->path_hardfile / sizeof (TCHAR)))
	    	return 1;
  	  return target_parse_option (p, option, value);
  	}
  	return 0;
  }

	for (i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		TCHAR tmp[20];
		_stprintf (tmp, _T("cdimage%d"), i);
		if (!_tcsicmp (option, tmp)) {
			if (!_tcsicmp (value, _T("autodetect"))) {
				p->cdslots[i].type = SCSI_UNIT_DEFAULT;
				p->cdslots[i].inuse = true;
				p->cdslots[i].name[0] = 0;
			} else {
				p->cdslots[i].delayed = false;
				TCHAR *next = _tcsrchr (value, ',');
				int type = SCSI_UNIT_DEFAULT;
				int mode = 0;
				int unitnum = 0;
				for (;;) {
					if (!next)
						break;
					*next++ = 0;
					TCHAR *next2 = _tcschr (next, ':');
					if (next2)
						*next2++ = 0;
					int tmpval = 0;
					if (!_tcsicmp (next, _T("delay"))) {
						p->cdslots[i].delayed = true;
						next = next2;
						if (!next)
							break;
						next2 = _tcschr (next, ':');
						if (next2)
							*next2++ = 0;
					}
					type = match_string (cdmodes, next);
					if (type < 0)
						type = SCSI_UNIT_DEFAULT;
					else
						type--;
					next = next2;
					if (!next)
						break;
					next2 = _tcschr (next, ':');
					if (next2)
						*next2++ = 0;
					mode = match_string (cdconmodes, next);
					if (mode < 0)
						mode = 0;
					next = next2;
					if (!next)
						break;
					next2 = _tcschr (next, ':');
					if (next2)
						*next2++ = 0;
					cfgfile_intval (option, next, tmp, &unitnum, 1);
				}
				if (_tcslen (value) > 0) {
					TCHAR *s = cfgfile_subst_path (UNEXPANDED, p->path_cd, value);
					_tcsncpy (p->cdslots[i].name, s, sizeof p->cdslots[i].name / sizeof (TCHAR));
					xfree (s);
				}
				p->cdslots[i].name[sizeof p->cdslots[i].name - 1] = 0;
				p->cdslots[i].inuse = true;
				p->cdslots[i].type = type;
			}
			// disable all following units
			i++;
			while (i < MAX_TOTAL_SCSI_DEVICES) {
				p->cdslots[i].type = SCSI_UNIT_DISABLED;
				i++;
			}
			return 1;
		}
	}

  if (cfgfile_intval (option, value, _T("sound_frequency"), &p->sound_freq, 1)
		|| cfgfile_intval (option, value, _T("sound_volume_cd"), &p->sound_volume_cd, 1)
	  || cfgfile_intval (option, value, _T("sound_stereo_separation"), &p->sound_stereo_separation, 1)
	  || cfgfile_intval (option, value, _T("sound_stereo_mixing_delay"), &p->sound_mixed_stereo_delay, 1)

	  || cfgfile_intval (option, value, _T("gfx_framerate"), &p->gfx_framerate, 1)
	  || cfgfile_intval (option, value, _T("gfx_width_windowed"), &p->gfx_size_win.width, 1)
	  || cfgfile_intval (option, value, _T("gfx_height_windowed"), &p->gfx_size_win.height, 1)
	  || cfgfile_intval (option, value, _T("gfx_width_fullscreen"), &p->gfx_size_fs.width, 1)
	  || cfgfile_intval (option, value, _T("gfx_height_fullscreen"), &p->gfx_size_fs.height, 1))

	  return 1;

#ifdef RASPBERRY
    if (cfgfile_intval (option, value, "gfx_correct_aspect", &p->gfx_correct_aspect, 1))
	    return 1;   

    if (cfgfile_intval (option, value, "gfx_fullscreen_ratio", &p->gfx_fullscreen_ratio, 1))
	    return 1;   
#endif

	if (cfgfile_string (option, value, _T("config_info"), p->info, sizeof p->info / sizeof (TCHAR))
	  || cfgfile_string (option, value, _T("config_description"), p->description, sizeof p->description / sizeof (TCHAR)))
	  return 1;

	if (cfgfile_yesno (option, value, _T("synchronize_clock"), &p->tod_hack)
		|| cfgfile_yesno (option, value, _T("bsdsocket_emu"), &p->socket_emu))
	  return 1;

  if (cfgfile_strval (option, value, _T("sound_output"), &p->produce_sound, soundmode1, 1)
	  || cfgfile_strval (option, value, _T("sound_output"), &p->produce_sound, soundmode2, 0)
	  || cfgfile_strval (option, value, _T("sound_interpol"), &p->sound_interpol, interpolmode, 0)
	  || cfgfile_strval (option, value, _T("sound_filter"), &p->sound_filter, soundfiltermode1, 0)
	  || cfgfile_strval (option, value, _T("sound_filter_type"), &p->sound_filter_type, soundfiltermode2, 0)
	  || cfgfile_strboolval (option, value, _T("use_gui"), &p->start_gui, guimode1, 1)
	  || cfgfile_strboolval (option, value, _T("use_gui"), &p->start_gui, guimode2, 1)
	  || cfgfile_strboolval (option, value, _T("use_gui"), &p->start_gui, guimode3, 0)
	  || cfgfile_strval (option, value, _T("gfx_resolution"), &p->gfx_resolution, lorestype1, 0)
	  || cfgfile_strval (option, value, _T("gfx_lores"), &p->gfx_resolution, lorestype2, 0)
	  || cfgfile_strval (option, value, _T("absolute_mouse"), &p->input_tablet, abspointers, 0))
    return 1;


  if (cfgfile_intval (option, value, "key_for_menu", &p->key_for_menu, 1))
    return 1;


	
  if(cfgfile_yesno (option, value, _T("show_leds"), &vb)) {
    p->leds_on_screen = vb;
    return 1;
  }

  if (_tcscmp (option, _T("gfx_width")) == 0 || _tcscmp (option, _T("gfx_height")) == 0) {
	  cfgfile_intval (option, value, _T("gfx_width"), &p->gfx_size.width, 1);
	  cfgfile_intval (option, value, _T("gfx_height"), &p->gfx_size.height, 1);
	  return 1;
  }

	if (_tcscmp (option, _T("joyportfriendlyname0")) == 0 || _tcscmp (option, _T("joyportfriendlyname1")) == 0) {
		inputdevice_joyport_config (p, value, _tcscmp (option, _T("joyportfriendlyname0")) == 0 ? 0 : 1, -1, 2);
		return 1;
	}
	if (_tcscmp (option, _T("joyportfriendlyname2")) == 0 || _tcscmp (option, _T("joyportfriendlyname3")) == 0) {
		inputdevice_joyport_config (p, value, _tcscmp (option, _T("joyportfriendlyname2")) == 0 ? 2 : 3, -1, 2);
		return 1;
	}
	if (_tcscmp (option, _T("joyportname0")) == 0 || _tcscmp (option, _T("joyportname1")) == 0) {
		inputdevice_joyport_config (p, value, _tcscmp (option, _T("joyportname0")) == 0 ? 0 : 1, -1, 1);
		return 1;
	}
	if (_tcscmp (option, _T("joyportname2")) == 0 || _tcscmp (option, _T("joyportname3")) == 0) {
		inputdevice_joyport_config (p, value, _tcscmp (option, _T("joyportname2")) == 0 ? 2 : 3, -1, 1);
		return 1;
	}
	if (_tcscmp (option, _T("joyport0")) == 0 || _tcscmp (option, _T("joyport1")) == 0) {
		inputdevice_joyport_config (p, value, _tcscmp (option, _T("joyport0")) == 0 ? 0 : 1, -1, 0);
		return 1;
	}
	if (_tcscmp (option, _T("joyport2")) == 0 || _tcscmp (option, _T("joyport3")) == 0) {
		inputdevice_joyport_config (p, value, _tcscmp (option, _T("joyport2")) == 0 ? 2 : 3, -1, 0);
		return 1;
	}
	if (cfgfile_strval (option, value, _T("joyport0mode"), &p->jports[0].mode, joyportmodes, 0))
		return 1;
	if (cfgfile_strval (option, value, _T("joyport1mode"), &p->jports[1].mode, joyportmodes, 0))
		return 1;
	if (cfgfile_strval (option, value, _T("joyport2mode"), &p->jports[2].mode, joyportmodes, 0))
		return 1;
	if (cfgfile_strval (option, value, _T("joyport3mode"), &p->jports[3].mode, joyportmodes, 0))
		return 1;
	if (cfgfile_strval (option, value, _T("joyport0autofire"), &p->jports[0].autofire, joyaf, 0))
		return 1;
	if (cfgfile_strval (option, value, _T("joyport1autofire"), &p->jports[1].autofire, joyaf, 0))
		return 1;
	if (cfgfile_strval (option, value, _T("joyport2autofire"), &p->jports[2].autofire, joyaf, 0))
		return 1;
	if (cfgfile_strval (option, value, _T("joyport3autofire"), &p->jports[3].autofire, joyaf, 0))
		return 1;

  if (cfgfile_string (option, value, _T("statefile"), tmpbuf, sizeof (tmpbuf) / sizeof (TCHAR))) {
	  _tcscpy (savestate_fname, tmpbuf);
	  if (zfile_exists (savestate_fname)) {
	    savestate_state = STATE_DORESTORE;
  	} else {
	    int ok = 0;
	    if (savestate_fname[0]) {
    		for (;;) {
  		    TCHAR *p;
  		    if (my_existsdir (savestate_fname)) {
		        ok = 1;
		        break;
  		    }
		      p = _tcsrchr (savestate_fname, '\\');
		      if (!p)
		        p = _tcsrchr (savestate_fname, '/');
  		    if (!p)
		        break;
  		    *p = 0;
    		}
	    }
	    if (!ok)
    		savestate_fname[0] = 0;
  	}
	  return 1;
  }

  if (cfgfile_strval (option, value, _T("sound_channels"), &p->sound_stereo, stereomode, 1)) {
	  if (p->sound_stereo == SND_NONE) { /* "mixed stereo" compatibility hack */
	    p->sound_stereo = SND_STEREO;
	    p->sound_mixed_stereo_delay = 5;
	    p->sound_stereo_separation = 7;
	  }
	  return 1;
  }

  if (cfgfile_string (option, value, _T("config_version"), tmpbuf, sizeof (tmpbuf) / sizeof (TCHAR))) {
  	TCHAR *tmpp2;
	  tmpp = _tcschr (value, '.');
	  if (tmpp) {
	    *tmpp++ = 0;
	    tmpp2 = tmpp;
	    p->config_version = _tstol (tmpbuf) << 16;
	    tmpp = _tcschr (tmpp, '.');
	    if (tmpp) {
	    	*tmpp++ = 0;
	    	p->config_version |= _tstol (tmpp2) << 8;
	    	p->config_version |= _tstol (tmpp);
	    }
	  }
	  return 1;
  }

  return 0;
}

static struct uaedev_config_info *getuci(struct uae_prefs *p)
{
  if (p->mountitems < MOUNT_CONFIG_SIZE)
  	return &p->mountconfig[p->mountitems++];
  return NULL;
}

struct uaedev_config_info *add_filesys_config (struct uae_prefs *p, int index,
	TCHAR *devname, TCHAR *volname, TCHAR *rootdir, bool readonly,
	int secspertrack, int surfaces, int reserved,
	int blocksize, int bootpri, 
  TCHAR *filesysdir, int hdc, int flags) 
{
  struct uaedev_config_info *uci;
  int i;
  TCHAR *s;

	if (index < 0 && devname && _tcslen (devname) > 0) {
		for (i = 0; i < p->mountitems; i++) {
			if (p->mountconfig[i].devname && !_tcscmp (p->mountconfig[i].devname, devname))
				return 0;
		}
	}

  if (index < 0) {
	  uci = getuci(p);
	  uci->configoffset = -1;
  } else {
  	uci = &p->mountconfig[index];
  }
  if (!uci)
  	return 0;
  uci->ishdf = volname == NULL ? 1 : 0;
	_tcscpy (uci->devname, devname ? devname : _T(""));
	_tcscpy (uci->volname, volname ? volname : _T(""));
	_tcscpy (uci->rootdir, rootdir ? rootdir : _T(""));
	validatedevicename (uci->devname);
	validatevolumename (uci->volname);
  uci->readonly = readonly;
  uci->sectors = secspertrack;
  uci->surfaces = surfaces;
  uci->reserved = reserved;
  uci->blocksize = blocksize;
  uci->bootpri = bootpri;
  uci->donotmount = 0;
  uci->autoboot = 0;
  if (bootpri < -128)
  	uci->donotmount = 1;
  else if (bootpri >= -127)
  	uci->autoboot = 1;
  uci->controller = hdc;
	_tcscpy (uci->filesys, filesysdir ? filesysdir : _T(""));
  if (!uci->devname[0]) {
	  TCHAR base[32];
	  TCHAR base2[32];
	  int num = 0;
	  if (uci->rootdir[0] == 0 && !uci->ishdf)
		  _tcscpy (base, _T("RDH"));
	  else
		  _tcscpy (base, _T("DH"));
	  _tcscpy (base2, base);
	  for (i = 0; i < p->mountitems; i++) {
		  _stprintf (base2, _T("%s%d"), base, num);
      if (!_tcscmp(base2, p->mountconfig[i].devname)) {
		    num++;
		    i = -1;
		    continue;
      }
	  }
  	_tcscpy (uci->devname, base2);
  	validatedevicename (uci->devname);
  }
  s = filesys_createvolname (volname, rootdir, _T("Harddrive"));
  _tcscpy (uci->volname, s);
  xfree (s);
  return uci;
}

static int cfgfile_parse_newfilesys (struct uae_prefs *p, int nr, bool hdf, TCHAR *value)
{
  int secs, heads, reserved, bs, bp, hdcv;
  bool ro;
  TCHAR *dname = NULL, *aname = _T(""), *root = NULL, *fs = NULL, *hdc;
  TCHAR *tmpp = _tcschr (value, ',');
  TCHAR *str = NULL;

  config_newfilesystem = 1;
  if (tmpp == 0)
    goto invalid_fs;

  *tmpp++ = '\0';
  if (strcasecmp (value, _T("ro")) == 0)
    ro = true;
  else if (strcasecmp (value, _T("rw")) == 0)
    ro = false;
	else
    goto invalid_fs;
  secs = 0; heads = 0; reserved = 0; bs = 0; bp = 0;
  fs = 0; hdc = 0; hdcv = 0;

  value = tmpp;
  if (!hdf) {
    tmpp = _tcschr (value, ':');
    if (tmpp == 0)
  		goto empty_fs;
    *tmpp++ = 0;
    dname = value;
    aname = tmpp;
    tmpp = _tcschr (tmpp, ':');
    if (tmpp == 0)
  		goto empty_fs;
    *tmpp++ = 0;
    root = tmpp;
    tmpp = _tcschr (tmpp, ',');
    if (tmpp == 0)
  		goto empty_fs;
    *tmpp++ = 0;
    if (! getintval (&tmpp, &bp, 0))
  		goto empty_fs;
	} else {
    tmpp = _tcschr (value, ':');
    if (tmpp == 0)
	    goto invalid_fs;
    *tmpp++ = '\0';
    dname = value;
    root = tmpp;
    tmpp = _tcschr (tmpp, ',');
    if (tmpp == 0)
	    goto invalid_fs;
    *tmpp++ = 0;
    aname = 0;
    if (! getintval (&tmpp, &secs, ',')
	  || ! getintval (&tmpp, &heads, ',')
	  || ! getintval (&tmpp, &reserved, ',')
	  || ! getintval (&tmpp, &bs, ','))
	    goto invalid_fs;
    if (getintval2 (&tmpp, &bp, ',')) {
	    fs = tmpp;
	    tmpp = _tcschr (tmpp, ',');
	    if (tmpp != 0) {
	      *tmpp++ = 0;
	      hdc = tmpp;
	      if(_tcslen(hdc) >= 4 && !_tcsncmp(hdc, _T("ide"), 3)) {
	      	hdcv = hdc[3] - '0' + HD_CONTROLLER_IDE0;
	      	if (hdcv < HD_CONTROLLER_IDE0 || hdcv > HD_CONTROLLER_IDE3)
  			    hdcv = 0;
		    }
		    if(_tcslen(hdc) >= 5 && !_tcsncmp(hdc, _T("scsi"), 4)) {
    			hdcv = hdc[4] - '0' + HD_CONTROLLER_SCSI0;
    			if (hdcv < HD_CONTROLLER_SCSI0 || hdcv > HD_CONTROLLER_SCSI6)
  			    hdcv = 0;
		    }
		    if (_tcslen (hdc) >= 6 && !_tcsncmp (hdc, _T("scsram"), 6))
    			hdcv = HD_CONTROLLER_PCMCIA_SRAM;
				if (_tcslen (hdc) >= 5 && !_tcsncmp (hdc, _T("scide"), 6))
    			hdcv = HD_CONTROLLER_PCMCIA_IDE;
  		}
    }
	}
empty_fs:
	if (root) {
		if (_tcslen (root) > 3 && root[0] == 'H' && root[1] == 'D' && root[2] == '_') {
			root += 2;
			*root = ':';
		}
    str = cfgfile_subst_path (UNEXPANDED, p->path_hardfile, root);
	}
#ifdef FILESYS
	add_filesys_config (p, nr, dname, aname, str, ro, secs, heads, reserved, bs, bp, fs, hdcv, 0);
#endif
  xfree (str);
  return 1;

invalid_fs:
	write_log (_T("Invalid filesystem/hardfile specification.\n"));
	return 1;
}

static int cfgfile_parse_filesys (struct uae_prefs *p, const TCHAR *option, TCHAR *value)
{
	int i;

  for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
	  TCHAR tmp[100];
	  _stprintf (tmp, _T("uaehf%d"), i);
	  if (_tcscmp (option, tmp) == 0) {
			for (;;) {
				bool hdf = false;
				TCHAR *tmpp = _tcschr (value, ',');
				if (tmpp == NULL)
	        return 1;
				*tmpp++ = 0;
				if (strcasecmp (value, _T("hdf")) == 0) {
					hdf = true;
				} else if (strcasecmp (value, _T("dir")) != 0) {
					return 1;
				}
#if 0			// not yet
				return cfgfile_parse_newfilesys (p, i, hdf, tmpp);
#else
				return 1;
#endif
			}
			return 1;
		}
  }

  if (_tcscmp (option, _T("filesystem")) == 0
	|| _tcscmp (option, _T("hardfile")) == 0)
  {
  	int secs, heads, reserved, bs;
	  bool ro;
	  TCHAR *aname, *root;
	  TCHAR *tmpp = _tcschr (value, ',');
	  TCHAR *str;

	  if (config_newfilesystem)
	    return 1;

	  if (tmpp == 0)
	    goto invalid_fs;

	  *tmpp++ = '\0';
	  if (_tcscmp (value, _T("1")) == 0 || strcasecmp (value, _T("ro")) == 0
    || strcasecmp (value, _T("readonly")) == 0
    || strcasecmp (value, _T("read-only")) == 0)
	    ro = true;
  	else if (_tcscmp (value, _T("0")) == 0 || strcasecmp (value, _T("rw")) == 0
		|| strcasecmp (value, _T("readwrite")) == 0
		|| strcasecmp (value, _T("read-write")) == 0)
	    ro = false;
	  else
	    goto invalid_fs;
	  secs = 0; heads = 0; reserved = 0; bs = 0;

  	value = tmpp;
	  if (_tcscmp (option, _T("filesystem")) == 0) {
	    tmpp = _tcschr (value, ':');
      if (tmpp == 0)
    		goto invalid_fs;
      *tmpp++ = '\0';
      aname = value;
      root = tmpp;
	  } else {
	    if (! getintval (&value, &secs, ',')
		  || ! getintval (&value, &heads, ',')
		  || ! getintval (&value, &reserved, ',')
		  || ! getintval (&value, &bs, ','))
    		goto invalid_fs;
	    root = value;
	    aname = 0;
  	}
  	str = cfgfile_subst_path (UNEXPANDED, p->path_hardfile, root);
#ifdef FILESYS
	  add_filesys_config (p, -1, NULL, aname, str, ro, secs, heads, reserved, bs, 0, NULL, 0, 0);
#endif
	  free (str);
	  return 1;
invalid_fs:
  	write_log (_T("Invalid filesystem/hardfile specification.\n"));
  	return 1;

	}

	if (_tcscmp (option, _T("filesystem2")) == 0)
		return cfgfile_parse_newfilesys (p, -1, false, value);
	if (_tcscmp (option, _T("hardfile2")) == 0)
		return cfgfile_parse_newfilesys (p, -1, true, value);

	return 0;
}

static int cfgfile_parse_hardware (struct uae_prefs *p, const TCHAR *option, TCHAR *value)
{
  int tmpval, dummyint, i;
  TCHAR *section = 0;
  TCHAR tmpbuf[CONFIG_BLEN];

  if (cfgfile_yesno (option, value, _T("immediate_blits"), &p->immediate_blits)
	  || cfgfile_yesno (option, value, _T("fast_copper"), &p->fast_copper)
		|| cfgfile_yesno (option, value, _T("cd32cd"), &p->cs_cd32cd)
		|| cfgfile_yesno (option, value, _T("cd32c2p"), &p->cs_cd32c2p)
		|| cfgfile_yesno (option, value, _T("cd32nvram"), &p->cs_cd32nvram)
	  || cfgfile_yesno (option, value, _T("ntsc"), &p->ntscmode)
	  || cfgfile_yesno (option, value, _T("cpu_compatible"), &p->cpu_compatible)
	  || cfgfile_yesno (option, value, _T("cpu_24bit_addressing"), &p->address_space_24))
	  return 1;
  if (cfgfile_intval (option, value, _T("cachesize"), &p->cachesize, 1)
	  || cfgfile_intval (option, value, _T("chipset_refreshrate"), &p->chipset_refreshrate, 1)
	  || cfgfile_intval (option, value, _T("fastmem_size"), &p->fastmem_size, 0x100000)
	  || cfgfile_intval (option, value, _T("z3mem_size"), &p->z3fastmem_size, 0x100000)
	  || cfgfile_intval (option, value, _T("z3mem_start"), &p->z3fastmem_start, 1)
	  || cfgfile_intval (option, value, _T("bogomem_size"), &p->bogomem_size, 0x40000)
	  || cfgfile_intval (option, value, _T("gfxcard_size"), &p->rtgmem_size, 0x100000)
	  || cfgfile_strval (option, value, _T("gfxcard_type"), &p->rtgmem_type, rtgtype, 0)
	  || cfgfile_intval (option, value, _T("rtg_modes"), &p->picasso96_modeflags, 1)
	  || cfgfile_intval (option, value, _T("floppy_speed"), &p->floppy_speed, 1)
	  || cfgfile_intval (option, value, _T("floppy_write_length"), &p->floppy_write_length, 1)
	  || cfgfile_intval (option, value, _T("nr_floppies"), &p->nr_floppies, 1)
	  || cfgfile_intval (option, value, _T("floppy0type"), &p->floppyslots[0].dfxtype, 1)
	  || cfgfile_intval (option, value, _T("floppy1type"), &p->floppyslots[1].dfxtype, 1)
	  || cfgfile_intval (option, value, _T("floppy2type"), &p->floppyslots[2].dfxtype, 1)
	  || cfgfile_intval (option, value, _T("floppy3type"), &p->floppyslots[3].dfxtype, 1))
	  return 1;

  if (cfgfile_strval (option, value, _T("collision_level"), &p->collision_level, collmode, 0))
  	return 1;
  if (cfgfile_string (option, value, _T("kickstart_rom_file"), p->romfile, sizeof p->romfile)
	  || cfgfile_string (option, value, _T("kickstart_ext_rom_file"), p->romextfile, sizeof p->romextfile)
    || cfgfile_string (option, value, _T("flash_file"), p->flashfile, sizeof p->flashfile))
	  return 1;

  for (i = 0; i < 4; i++) {
	  _stprintf (tmpbuf, _T("floppy%d"), i);
	  if (cfgfile_path (option, value, tmpbuf, p->floppyslots[i].df, sizeof p->floppyslots[i].df / sizeof (TCHAR)))
	    return 1;
  }

  if (cfgfile_intval (option, value, _T("chipmem_size"), &dummyint, 1)) {
  	if (dummyint < 0)
	    p->chipmem_size = 0x20000; /* 128k, prototype support */
  	else if (dummyint == 0)
	    p->chipmem_size = 0x40000; /* 256k */
  	else
	    p->chipmem_size = dummyint * 0x80000;
  	return 1;
  }

  if (cfgfile_strval (option, value, _T("chipset"), &tmpval, csmode, 0)) {
    set_chipset_mask (p, tmpval);
    return 1;
  }

  if (cfgfile_string (option, value, _T("fpu_model"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) {
	  p->fpu_model = _tstol(tmpbuf);
	  return 1;
  }

	if (cfgfile_string (option, value, _T("cpu_model"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) {
	  p->cpu_model = _tstol(tmpbuf);
	  p->fpu_model = 0;
	  return 1;
  }

    /* old-style CPU configuration */
	if (cfgfile_string (option, value, _T("cpu_type"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) {
	  p->fpu_model = 0;
	  p->address_space_24 = 0;
	  p->cpu_model = 680000;
		if (!_tcscmp (tmpbuf, _T("68000"))) {
	    p->cpu_model = 68000;
		} else if (!_tcscmp (tmpbuf, _T("68010"))) {
	    p->cpu_model = 68010;
		} else if (!_tcscmp (tmpbuf, _T("68ec020"))) {
	    p->cpu_model = 68020;
	    p->address_space_24 = 1;
		} else if (!_tcscmp (tmpbuf, _T("68020"))) {
	    p->cpu_model = 68020;
		} else if (!_tcscmp (tmpbuf, _T("68ec020/68881"))) {
	    p->cpu_model = 68020;
	    p->fpu_model = 68881;
	    p->address_space_24 = 1;
		} else if (!_tcscmp (tmpbuf, _T("68020/68881"))) {
	    p->cpu_model = 68020;
	    p->fpu_model = 68881;
		} else if (!_tcscmp (tmpbuf, _T("68040"))) {
	    p->cpu_model = 68040;
	    p->fpu_model = 68040;
		} else if (!_tcscmp (tmpbuf, _T("68060"))) {
	    p->cpu_model = 68060;
	    p->fpu_model = 68060;
  	}
	  return 1;
  }
    
  if (p->config_version < (21 << 16)) {
  	if (cfgfile_strval (option, value, _T("cpu_speed"), &p->m68k_speed, speedmode, 1)
	    /* Broken earlier versions used to write this out as a string.  */
	    || cfgfile_strval (option, value, _T("finegraincpu_speed"), &p->m68k_speed, speedmode, 1))
  	{
	    p->m68k_speed--;
	    return 1;
  	}
  }

  if (cfgfile_intval (option, value, _T("cpu_speed"), &p->m68k_speed, 1)) {
    p->m68k_speed *= CYCLE_UNIT;
  	return 1;
  }

  if (cfgfile_intval (option, value, _T("finegrain_cpu_speed"), &p->m68k_speed, 1)) {
  	if (OFFICIAL_CYCLE_UNIT > CYCLE_UNIT) {
	    int factor = OFFICIAL_CYCLE_UNIT / CYCLE_UNIT;
	    p->m68k_speed = (p->m68k_speed + factor - 1) / factor;
  	}
    if (strcasecmp (value, _T("max")) == 0)
      p->m68k_speed = -1;
  	return 1;
  }

	if (cfgfile_parse_filesys (p, option, value))
		return 1;

  return 0;
}

int cfgfile_parse_option (struct uae_prefs *p, TCHAR *option, TCHAR *value, int type)
{
  if (!_tcscmp (option, _T("config_hardware")))
  	return 1;
  if (!_tcscmp (option, _T("config_host")))
  	return 1;
  if (type == 0 || (type & CONFIG_TYPE_HARDWARE)) {
  	if (cfgfile_parse_hardware (p, option, value))
	    return 1;
  }
  if (type == 0 || (type & CONFIG_TYPE_HOST)) {
  	if (cfgfile_parse_host (p, option, value))
	    return 1;
  }
	if (type > 0 && (type & (CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST)) != (CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST))
  	return 1;
  return 0;
}

static int cfgfile_separate_linea (char *line, TCHAR *line1b, TCHAR *line2b)
{
  char *line1, *line2;
  int i;

  line1 = line;
  line1 += strspn (line1, "\t \r\n");
  if (*line1 == ';')
	  return 0;
  line2 = strchr (line, '=');
  if (! line2) {
  	write_log ("CFGFILE: line was incomplete with only %s\n", line1);
  	return 0;
  }
  *line2++ = '\0';

  /* Get rid of whitespace.  */
  i = strlen (line2);
  while (i > 0 && (line2[i - 1] == '\t' || line2[i - 1] == ' '
    || line2[i - 1] == '\r' || line2[i - 1] == '\n'))
	  line2[--i] = '\0';
  line2 += strspn (line2, "\t \r\n");

  i = strlen (line);
  while (i > 0 && (line[i - 1] == '\t' || line[i - 1] == ' '
    || line[i - 1] == '\r' || line[i - 1] == '\n'))
  	line[--i] = '\0';
  line += strspn (line, "\t \r\n");

  _tcscpy (line1b, line);
  _tcscpy (line2b, line2);

  return 1;
}

static int cfgfile_separate_line (TCHAR *line, TCHAR *line1b, TCHAR *line2b)
{
  return  cfgfile_separate_linea(line, line1b, line2b);
}

static int isobsolete (TCHAR *s)
{
  int i = 0;
  while (obsolete[i]) {
  	if (!strcasecmp (s, obsolete[i])) {
	    write_log (_T("obsolete config entry '%s'\n"), s);
	    return 1;
  	}
  	i++;
  }
  if (_tcslen (s) > 2 && !_tcsncmp (s, _T("w."), 2))
  	return 1;
  if (_tcslen (s) >= 10 && !_tcsncmp (s, _T("gfx_opengl"), 10)) {
  	write_log (_T("obsolete config entry '%s\n"), s);
  	return 1;
  }
  if (_tcslen (s) >= 6 && !_tcsncmp (s, _T("gfx_3d"), 6)) {
    write_log (_T("obsolete config entry '%s\n"), s);
    return 1;
  }
  return 0;
}

static void cfgfile_parse_separated_line (struct uae_prefs *p, TCHAR *line1b, TCHAR *line2b, int type)
{
  TCHAR line3b[CONFIG_BLEN], line4b[CONFIG_BLEN];
  struct strlist *sl;
  int ret;

  _tcscpy (line3b, line1b);
  _tcscpy (line4b, line2b);
  ret = cfgfile_parse_option (p, line1b, line2b, type);
  if (!isobsolete (line3b)) {
  	for (sl = p->all_lines; sl; sl = sl->next) {
	    if (sl->option && !strcasecmp (line1b, sl->option)) break;
  	}
  	if (!sl) {
	    struct strlist *u = xcalloc (struct strlist, 1);
	    u->option = my_strdup(line3b);
	    u->value = my_strdup(line4b);
	    u->next = p->all_lines;
	    p->all_lines = u;
	    if (!ret) {
		    u->unknown = 1;
		    write_log (_T("unknown config entry: '%s=%s'\n"), u->option, u->value);
	    }
  	}
  }
}

void cfgfile_parse_line (struct uae_prefs *p, TCHAR *line, int type)
{
  TCHAR line1b[CONFIG_BLEN], line2b[CONFIG_BLEN];

  if (!cfgfile_separate_line (line, line1b, line2b))
  	return;
  cfgfile_parse_separated_line (p, line1b, line2b, type);
}

static void subst (TCHAR *p, TCHAR *f, int n)
{
  TCHAR *str = cfgfile_subst_path (UNEXPANDED, p, f);
  _tcsncpy (f, str, n - 1);
  f[n - 1] = '\0';
  free (str);
}

static char *cfg_fgets (char *line, int max, struct zfile *fh)
{
  if (fh)
  	return zfile_fgetsa (line, max, fh);
  return 0;
}

static int cfgfile_load_2 (struct uae_prefs *p, const TCHAR *filename, bool real, int *type)
{
  int i;
  struct zfile *fh;
  char linea[CONFIG_BLEN];
  TCHAR line[CONFIG_BLEN], line1b[CONFIG_BLEN], line2b[CONFIG_BLEN];
  struct strlist *sl;
  bool type1 = false, type2 = false;
  int askedtype = 0;

  if (type) {
	  askedtype = *type;
	  *type = 0;
  }
  if (real) {
	  p->config_version = 0;
	  config_newfilesystem = 0;
	  //reset_inputdevice_config (p);
  }

  fh = zfile_fopen (filename, _T("r"), ZFD_NORMAL);
  if (! fh)
  	return 0;

  while (cfg_fgets (linea, sizeof (linea), fh) != 0) {
  	trimwsa (linea);
  	if (strlen (linea) > 0) {
			if (linea[0] == '#' || linea[0] == ';') {
				struct strlist *u = xcalloc (struct strlist, 1);
				u->option = NULL;
				u->value = my_strdup (linea);
				u->unknown = 1;
				u->next = p->all_lines;
				p->all_lines = u;
		    continue;
			}
	    if (!cfgfile_separate_linea (linea, line1b, line2b))
    		continue;
	    type1 = type2 = 0;
	    if (cfgfile_yesno (line1b, line2b, _T("config_hardware"), &type1) ||
        cfgfile_yesno (line1b, line2b, _T("config_host"), &type2)) {
  	    	if (type1 && type)
	    	    *type |= CONFIG_TYPE_HARDWARE;
  	    	if (type2 && type)
    		    *type |= CONFIG_TYPE_HOST;
      		continue;
	    }
	    if (real) {
    		cfgfile_parse_separated_line (p, line1b, line2b, askedtype);
	    } else {
    		cfgfile_string (line1b, line2b, _T("config_description"), p->description, sizeof p->description / sizeof (TCHAR));
	    }
	  }
  }

  if (type && *type == 0)
  	*type = CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST;
  zfile_fclose (fh);

  if (!real)
  	return 1;

  for (sl = temp_lines; sl; sl = sl->next) {
  	_stprintf (line, _T("%s=%s"), sl->option, sl->value);
  	cfgfile_parse_line (p, line, 0);
  }

  for (i = 0; i < 4; i++) {
  	subst (p->path_floppy, p->floppyslots[i].df, sizeof p->floppyslots[i].df / sizeof (TCHAR));
  	if(i >= p->nr_floppies)
  	  p->floppyslots[i].dfxtype = DRV_NONE;
  }
  subst (p->path_rom, p->romfile, sizeof p->romfile / sizeof (TCHAR));
  subst (p->path_rom, p->romextfile, sizeof p->romextfile / sizeof (TCHAR));

  return 1;
}

int cfgfile_load (struct uae_prefs *p, const TCHAR *filename, int *type, int ignorelink, int userconfig)
{
  int v;
  TCHAR tmp[MAX_DPATH];
  int type2;
  static int recursive;

  if (recursive > 1)
  	return 0;
  recursive++;
	write_log (_T("load config '%s':%d\n"), filename, type ? *type : -1);
  v = cfgfile_load_2 (p, filename, 1, type);
  if (!v) {
		write_log (_T("load failed\n"));
	  goto end;
  }
end:
  recursive--;
  fixup_prefs (p);
  return v;
}

int cfgfile_save (struct uae_prefs *p, const TCHAR *filename, int type)
{
  struct zfile *fh;

  fh = zfile_fopen (filename, _T("w"), ZFD_NORMAL);
  if (! fh)
  	return 0;

  if (!type)
  	type = CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST;
  cfgfile_save_options (fh, p, type);
  zfile_fclose (fh);
  return 1;
}


int cfgfile_get_description (const TCHAR *filename, TCHAR *description)
{
  int result = 0;
  struct uae_prefs *p = xmalloc (struct uae_prefs, 1);

  p->description[0] = 0;
  if (cfgfile_load_2 (p, filename, 0, 0)) {
  	result = 1;
	  if (description)
	    _tcscpy (description, p->description);
  }
  xfree (p);
  return result;
}


int cfgfile_configuration_change(int v)
{
  static int mode;
  if (v >= 0)
  	mode = v;
  return mode;
}

static void parse_sound_spec (struct uae_prefs *p, const TCHAR *spec)
{
  TCHAR *x0 = my_strdup (spec);
  TCHAR *x1, *x2 = NULL, *x3 = NULL, *x4 = NULL, *x5 = NULL;

  x1 = _tcschr (x0, ':');
  if (x1 != NULL) {
	  *x1++ = '\0';
	  x2 = _tcschr (x1 + 1, ':');
	  if (x2 != NULL) {
	    *x2++ = '\0';
	    x3 = _tcschr (x2 + 1, ':');
	    if (x3 != NULL) {
		    *x3++ = '\0';
		    x4 = _tcschr (x3 + 1, ':');
		    if (x4 != NULL) {
		      *x4++ = '\0';
		      x5 = _tcschr (x4 + 1, ':');
    		}
	    }
  	}
  }
  p->produce_sound = _tstoi (x0);
  if (x1) {
  	p->sound_stereo_separation = 0;
  	if (*x1 == 'S') {
	    p->sound_stereo = SND_STEREO;
	    p->sound_stereo_separation = 7;
  	} else if (*x1 == 's')
	    p->sound_stereo = SND_STEREO;
  	else
	    p->sound_stereo = SND_MONO;
  }
  if (x3)
  	p->sound_freq = _tstoi (x3);
  free (x0);
}

static void parse_joy_spec (struct uae_prefs *p, const TCHAR *spec)
{
	int v0 = 2, v1 = 0;
	if (_tcslen(spec) != 2)
		goto bad;

	switch (spec[0]) {
	case '0': v0 = JSEM_JOYS; break;
	case '1': v0 = JSEM_JOYS + 1; break;
	case 'M': case 'm': v0 = JSEM_MICE; break;
	case 'A': case 'a': v0 = JSEM_KBDLAYOUT; break;
	case 'B': case 'b': v0 = JSEM_KBDLAYOUT + 1; break;
	case 'C': case 'c': v0 = JSEM_KBDLAYOUT + 2; break;
	default: goto bad;
	}

	switch (spec[1]) {
	case '0': v1 = JSEM_JOYS; break;
	case '1': v1 = JSEM_JOYS + 1; break;
	case 'M': case 'm': v1 = JSEM_MICE; break;
	case 'A': case 'a': v1 = JSEM_KBDLAYOUT; break;
	case 'B': case 'b': v1 = JSEM_KBDLAYOUT + 1; break;
	case 'C': case 'c': v1 = JSEM_KBDLAYOUT + 2; break;
	default: goto bad;
	}
	if (v0 == v1)
		goto bad;
	/* Let's scare Pascal programmers */
	if (0)
bad:
	write_log (_T("Bad joystick mode specification. Use -J xy, where x and y\n")
		_T("can be 0 for joystick 0, 1 for joystick 1, M for mouse, and\n")
		_T("a, b or c for different keyboard settings.\n"));

	p->jports[0].id = v0;
	p->jports[1].id = v1;
}

static void parse_filesys_spec (struct uae_prefs *p, bool readonly, const TCHAR *spec)
{
  TCHAR buf[256];
  TCHAR *s2;

  _tcsncpy (buf, spec, 255); buf[255] = 0;
  s2 = _tcschr (buf, ':');
  if (s2) {
  	*s2++ = '\0';
#ifdef __DOS__
	  {
      TCHAR *tmp;

      while ((tmp = _tcschr (s2, '\\')))
    		*tmp = '/';
	  }
#endif
#ifdef FILESYS
  	add_filesys_config (p, -1, NULL, buf, s2, readonly, 0, 0, 0, 0, 0, 0, 0, 0);
#endif
  } else {
		write_log (_T("Usage: [-m | -M] VOLNAME:mount_point\n"));
  }
}

static void parse_hardfile_spec (struct uae_prefs *p, const TCHAR *spec)
{
  TCHAR *x0 = my_strdup (spec);
  TCHAR *x1, *x2, *x3, *x4;

  x1 = _tcschr (x0, ':');
  if (x1 == NULL)
  	goto argh;
  *x1++ = '\0';
  x2 = _tcschr (x1 + 1, ':');
  if (x2 == NULL)
  	goto argh;
  *x2++ = '\0';
  x3 = _tcschr (x2 + 1, ':');
  if (x3 == NULL)
  	goto argh;
  *x3++ = '\0';
  x4 = _tcschr (x3 + 1, ':');
  if (x4 == NULL)
  	goto argh;
  *x4++ = '\0';
#ifdef FILESYS
  add_filesys_config (p, -1, NULL, NULL, x4, 0, _tstoi (x0), _tstoi (x1), _tstoi (x2), _tstoi (x3), 0, 0, 0, 0);
#endif

  free (x0);
  return;

 argh:
  free (x0);
	write_log (_T("Bad hardfile parameter specified - type \"uae -h\" for help.\n"));
  return;
}

static void parse_cpu_specs (struct uae_prefs *p, const TCHAR *spec)
{
  if (*spec < '0' || *spec > '4') {
		write_log (_T("CPU parameter string must begin with '0', '1', '2', '3' or '4'.\n"));
	  return;
  }

  p->cpu_model = (*spec++) * 10 + 68000;
  p->address_space_24 = p->cpu_model < 68020;
  p->cpu_compatible = 0;
  while (*spec != '\0') {
	  switch (*spec) {
	  case 'a':
	    if (p->cpu_model < 68020)
				write_log (_T("In 68000/68010 emulation, the address space is always 24 bit.\n"));
	    else if (p->cpu_model >= 68040)
				write_log (_T("In 68040/060 emulation, the address space is always 32 bit.\n"));
	    else
		    p->address_space_24 = 1;
	    break;
	  case 'c':
	    if (p->cpu_model != 68000)
				write_log (_T("The more compatible CPU emulation is only available for 68000\n")
				_T("emulation, not for 68010 upwards.\n"));
	    else
		    p->cpu_compatible = 1;
	    break;
	  default:
			write_log (_T("Bad CPU parameter specified - type \"uae -h\" for help.\n"));
	    break;
	  }
	  spec++;
  }
}

static void cmdpath (TCHAR *dst, const TCHAR *src, int maxsz)
{
	TCHAR *s = target_expand_environment (src);
	_tcsncpy (dst, s, maxsz);
	dst[maxsz] = 0;
	xfree (s);
}

/* Returns the number of args used up (0 or 1).  */
int parse_cmdline_option (struct uae_prefs *p, TCHAR c, const TCHAR *arg)
{
  struct strlist *u = xcalloc (struct strlist, 1);
	const TCHAR arg_required[] = _T("0123rKpImWSAJwNCZUFcblOdHRv");

  if (_tcschr (arg_required, c) && ! arg) {
		write_log (_T("Missing argument for option `-%c'!\n"), c);
	  return 0;
  }

  u->option = xmalloc (TCHAR, 2);
  u->option[0] = c;
  u->option[1] = 0;
  u->value = my_strdup(arg);
  u->next = p->all_lines;
  p->all_lines = u;

  switch (c) {
    case '0': cmdpath (p->floppyslots[0].df, arg, 255); break;
    case '1': cmdpath (p->floppyslots[1].df, arg, 255); break;
    case '2': cmdpath (p->floppyslots[2].df, arg, 255); break;
    case '3': cmdpath (p->floppyslots[3].df, arg, 255); break;
    case 'r': cmdpath (p->romfile, arg, 255); break;
    case 'K': cmdpath (p->romextfile, arg, 255); break;
    case 'm': case 'M': parse_filesys_spec (p, c == 'M', arg); break;
    case 'W': parse_hardfile_spec (p, arg); break;
    case 'S': parse_sound_spec (p, arg); break;
    case 'R': p->gfx_framerate = _tstoi (arg); break;
	  case 'J': parse_joy_spec (p, arg); break;

    case 'w': p->m68k_speed = _tstoi (arg); break;

    case 'G': p->start_gui = 0; break;

    case 'n':
	    if (_tcschr (arg, 'i') != 0)
	      p->immediate_blits = 1;
	    break;

    case 'v':
	    set_chipset_mask (p, _tstoi (arg));
	    break;

    case 'C':
	    parse_cpu_specs (p, arg);
	    break;

    case 'Z':
	    p->z3fastmem_size = _tstoi (arg) * 0x100000;
	    break;

    case 'U':
	    p->rtgmem_size = _tstoi (arg) * 0x100000;
	    break;

    case 'F':
	    p->fastmem_size = _tstoi (arg) * 0x100000;
	    break;

    case 'b':
	    p->bogomem_size = _tstoi (arg) * 0x40000;
	    break;

    case 'c':
	    p->chipmem_size = _tstoi (arg) * 0x80000;
	    break;

    default:
		  write_log (_T("Unknown option `-%c'!\n"), c);
	    break;
  }
  return !! _tcschr (arg_required, c);
}

void cfgfile_addcfgparam (TCHAR *line)
{
  struct strlist *u;
  TCHAR line1b[CONFIG_BLEN], line2b[CONFIG_BLEN];

  if (!line) {
	  struct strlist **ps = &temp_lines;
	  while (*ps) {
	    struct strlist *s = *ps;
	    *ps = s->next;
	    xfree (s->value);
	    xfree (s->option);
	    xfree (s);
  	}
  	temp_lines = 0;
  	return;
  }
  if (!cfgfile_separate_line (line, line1b, line2b))
  	return;
  u = xcalloc (struct strlist, 1);
  u->option = my_strdup(line1b);
  u->value = my_strdup(line2b);
  u->next = temp_lines;
  temp_lines = u;
}

void default_prefs (struct uae_prefs *p, int type)
{
  int i;
  TCHAR zero = 0;
  struct zfile *f;

	reset_inputdevice_config (p);
  memset (p, 0, sizeof (struct uae_prefs));
  _tcscpy (p->description, _T("UAE default configuration"));

  p->start_gui = 1;

  p->all_lines = 0;

	memset (&p->jports[0], 0, sizeof (struct jport));
	memset (&p->jports[1], 0, sizeof (struct jport));
	memset (&p->jports[2], 0, sizeof (struct jport));
	memset (&p->jports[3], 0, sizeof (struct jport));
	p->jports[0].id = JSEM_MICE;
	p->jports[1].id = JSEM_JOYS;
	p->jports[2].id = -1;
	p->jports[3].id = -1;

  p->produce_sound = 3;
  p->sound_stereo = SND_STEREO;
  p->sound_stereo_separation = 7;
  p->sound_mixed_stereo_delay = 0;
  p->sound_freq = DEFAULT_SOUND_FREQ;
  p->sound_interpol = 0;
  p->sound_filter = FILTER_SOUND_OFF;
  p->sound_filter_type = 0;
	p->sound_volume_cd = 20;

  p->cachesize = DEFAULT_JIT_CACHE_SIZE;

  for (i = 0;i < 10; i++)
	  p->optcount[i] = -1;
  p->optcount[0] = 4;	/* How often a block has to be executed before it is translated */
  p->optcount[1] = 0;	/* How often to use the naive translation */
  p->optcount[2] = 0;
  p->optcount[3] = 0;
  p->optcount[4] = 0;
  p->optcount[5] = 0;

  p->gfx_framerate = 0;
  p->gfx_size_fs.width = 640;
  p->gfx_size_fs.height = 480;
  p->gfx_size_win.width = 320;
  p->gfx_size_win.height = 240;
#ifdef RASPBERRY
  p->gfx_size.width = 640;
  p->gfx_size.height = 262;
#else
  p->gfx_size.width = 320;
  p->gfx_size.height = 240;
#endif
  p->gfx_resolution = RES_LORES;
#ifdef RASPBERRY
  p->gfx_correct_aspect = 1;
  p->gfx_fullscreen_ratio = 100;
#endif
  p->immediate_blits = 0;
  p->chipset_refreshrate = 50;
  p->collision_level = 2;
  p->leds_on_screen = 0;
  p->fast_copper = 1;
  p->tod_hack = 1;

	p->cs_cd32c2p = p->cs_cd32cd = p->cs_cd32nvram = false;

  _tcscpy (p->floppyslots[0].df, _T(""));
  _tcscpy (p->floppyslots[1].df, _T(""));
  _tcscpy (p->floppyslots[2].df, _T(""));
  _tcscpy (p->floppyslots[3].df, _T(""));

  // Choose automatically first rom.
  if (lstAvailableROMs.size() >= 1)
  {
    strncpy(currprefs.romfile,lstAvailableROMs[0]->Path,255);
    //_tcscpy(currprefs.romfile,lstAvailableROMs[0]->Path,255);
  }
  else
  _tcscpy (p->romfile, _T("kick.rom"));

  _tcscpy (p->romextfile, _T(""));
	_tcscpy (p->flashfile, _T(""));

  sprintf (p->path_rom, _T("%s/kickstarts/"), start_path_data);
  sprintf (p->path_floppy, _T("%s/disks/"), start_path_data);
  sprintf (p->path_hardfile, _T("%s/"), start_path_data);
  sprintf (p->path_cd, _T("%s/cd32/"), start_path_data);

  p->fpu_model = 0;
  p->cpu_model = 68000;
  p->m68k_speed = 0;
  p->cpu_compatible = 0;
  p->address_space_24 = 1;
  p->chipset_mask = CSMASK_ECS_AGNUS;
  p->ntscmode = 0;

  p->fastmem_size = 0x00000000;
  p->z3fastmem_size = 0x00000000;
  p->z3fastmem_start = z3_start_adr;
  p->chipmem_size = 0x00100000;
  p->bogomem_size = 0x00000000;
  p->rtgmem_size = 0x00000000;
	p->rtgmem_type = 1;

  p->nr_floppies = 2;
  p->floppyslots[0].dfxtype = DRV_35_DD;
  p->floppyslots[1].dfxtype = DRV_35_DD;
  p->floppyslots[2].dfxtype = DRV_NONE;
  p->floppyslots[3].dfxtype = DRV_NONE;
  p->floppy_speed = 100;
  p->floppy_write_length = 0;
  
	p->socket_emu = 0;

  p->input_tablet = TABLET_OFF;

  p->key_for_menu = SDLK_F12;

  inputdevice_default_prefs (p);

	blkdev_default_prefs (p);

  target_default_options (p, type);
}


int bip_a500 (struct uae_prefs *p, int rom)
{
  int roms[4];

	if(rom == 130)
  {
  	roms[0] = 6;
  	roms[1] = 5;
  	roms[2] = 4;
    roms[3] = -1;
  }
  else
  {
  	roms[0] = 5;
  	roms[1] = 4;
  	roms[2] = 3;
    roms[3] = -1;
  }
  p->chipmem_size = 0x00080000;
	p->chipset_mask = 0;
  p->cpu_compatible = 0;
  p->fast_copper = 0;
  p->nr_floppies = 1;
	p->floppyslots[1].dfxtype = DRV_NONE;
  return configure_rom (p, roms, 0);
}


int bip_a500plus (struct uae_prefs *p, int rom)
{
  int roms[4];

	if(rom == 130)
  {
  	roms[0] = 6;
  	roms[1] = 5;
  	roms[2] = 4;
    roms[3] = -1;
  }
  else
  {
  	roms[0] = 7;
  	roms[1] = 6;
  	roms[2] = 5;
    roms[3] = -1;
  }
  p->chipmem_size = 0x00100000;
	p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
  p->cpu_compatible = 0;
  p->fast_copper = 0;
  p->nr_floppies = 1;
	p->floppyslots[1].dfxtype = DRV_NONE;
  return configure_rom (p, roms, 0);
}


int bip_a1200 (struct uae_prefs *p, int rom)
{
	int roms[4];

	if(rom == 310)
  {
  	roms[0] = 15;
  	roms[1] = 11;
  	roms[2] = 31;
    roms[3] = -1;
  }
  else
  {
  	roms[0] = 11;
  	roms[1] = 15;
  	roms[2] = 31;
  	roms[3] = -1;
  }
	p->cpu_model = 68020;
	p->address_space_24 = 1;
	p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA;
	p->chipmem_size = 0x200000;
	p->bogomem_size = 0;
	p->m68k_speed = M68K_SPEED_14MHZ_CYCLES;

  p->nr_floppies = 1;
	p->floppyslots[1].dfxtype = DRV_NONE;

	return configure_rom (p, roms, 0);
}


int bip_a2000 (struct uae_prefs *p, int rom)
{
  int roms[4];

	if(rom == 130)
  {
  	roms[0] = 6;
  	roms[1] = 5;
  	roms[2] = 4;
    roms[3] = -1;
  }
  else
  {
  	roms[0] = 5;
  	roms[1] = 4;
  	roms[2] = 3;
    roms[3] = -1;
  }
  p->chipmem_size = 0x00080000;
  p->bogomem_size = 0x00080000;
	p->chipset_mask = 0;
  p->cpu_compatible = 0;
  p->fast_copper = 0;
  p->nr_floppies = 1;
	p->floppyslots[1].dfxtype = DRV_NONE;
  return configure_rom (p, roms, 0);
}


int bip_a4000 (struct uae_prefs *p, int rom)
{
	int roms[4];

	roms[0] = 15;
	roms[1] = 14;
	roms[2] = 11;
	roms[3] = -1;

	p->bogomem_size = 0;
	p->chipmem_size = 0x200000;
	p->cpu_model = 68030;
	p->fpu_model = 68882;
	p->address_space_24 = 0;
  p->cpu_compatible = 0;
	p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA;
	p->m68k_speed = -1;
	p->cachesize = 8192;

  p->nr_floppies = 2;
	p->floppyslots[0].dfxtype = DRV_35_HD;
	p->floppyslots[1].dfxtype = DRV_35_HD;

	return configure_rom (p, roms, 0);
}


int bip_cd32 (struct uae_prefs *p, int rom)
{
	int roms[2];

	roms[0] = 64;
	roms[1] = -1;
	if (!configure_rom (p, roms, 0)) {
		roms[0] = 18;
		roms[1] = -1;
		if (!configure_rom (p, roms, 0))
			return 0;
		roms[0] = 19;
		if (!configure_rom (p, roms, 0))
			return 0;
	}
//	if (config > 0) {
//		roms[0] = 23;
//		if (!configure_rom (p, roms, 0))
//			return 0;
//	}

	p->cpu_model = 68020;
	p->address_space_24 = 1;
	p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA;
	p->chipmem_size = 0x200000;
	p->bogomem_size = 0;
	p->m68k_speed = M68K_SPEED_14MHZ_CYCLES;

	p->cs_cd32c2p = 1;
	p->cs_cd32cd = 1;
	p->cs_cd32nvram = 1;
	p->nr_floppies = 0;
	p->floppyslots[0].dfxtype = DRV_NONE;
	p->floppyslots[1].dfxtype = DRV_NONE;
	fetch_datapath (p->flashfile, sizeof (p->flashfile) / sizeof (TCHAR));
	_tcscat (p->flashfile, _T("cd32.nvr"));

	p->cdslots[0].inuse = true;
	p->cdslots[0].type = SCSI_UNIT_IMAGE;
	
	p->gfx_size.width = 384;
	p->gfx_size.height = 256;

	return 1;
}
