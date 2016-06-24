/*
 * This file is part of nmealib.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <nmealib/gpvtg.h>

#include <nmealib/context.h>
#include <nmealib/gmath.h>
#include <nmealib/info.h>
#include <nmealib/parse.h>
#include <nmealib/sentence.h>
#include <nmealib/tok.h>
#include <nmealib/validate.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool nmeaGPVTGparse(const char *s, const size_t sz, nmeaGPVTG *pack) {
  bool speedK = false;
  bool speedN = false;
  int fieldCount;

  if (!pack) {
    return false;
  }

  if (!s) {
    goto err;
  }

  nmeaTraceBuffer(s, sz);

  /* Clear before parsing, to be able to detect absent fields */
  memset(pack, 0, sizeof(*pack));
  pack->track = NAN;
  pack->mtrack = NAN;
  pack->spn = NAN;
  pack->spk = NAN;

  /* parse */
  fieldCount = nmea_scanf(s, sz, //
      "$GPVTG,%f,%c,%f,%c,%f,%c,%f,%c*", //
      &pack->track, //
      &pack->track_t, //
      &pack->mtrack, //
      &pack->mtrack_m, //
      &pack->spn, //
      &pack->spn_n, //
      &pack->spk, //
      &pack->spk_k);

  /* see that there are enough tokens */
  if (fieldCount != 8) {
    nmeaError("GPVTG parse error: need 8 tokens, got %d in '%s'", fieldCount, s);
    return false;
  }

  /* determine which fields are present and validate them */

  if (!isnan(pack->track) && (pack->track_t)) {
    pack->track_t = toupper(pack->track_t);
    if (pack->track_t != 'T') {
      nmeaError("GPVTG parse error: invalid track unit, got '%c', expected 'T'", pack->track_t);
      return false;
    }

    nmea_INFO_set_present(&pack->present, TRACK);
  } else {
    pack->track = 0.0;
    pack->track_t = '\0';
  }

  if (!isnan(pack->mtrack) && (pack->mtrack_m)) {
    pack->mtrack_m = toupper(pack->mtrack_m);
    if (pack->mtrack_m != 'M') {
      nmeaError("GPVTG parse error: invalid mtrack unit, got '%c', expected 'M'", pack->mtrack_m);
      return false;
    }

    nmea_INFO_set_present(&pack->present, MTRACK);
  } else {
    pack->mtrack = 0.0;
    pack->mtrack_m = '\0';
  }

  if (!isnan(pack->spn) && (pack->spn_n)) {
    pack->spn_n = toupper(pack->spn_n);
    if (pack->spn_n != 'N') {
      nmeaError("GPVTG parse error: invalid knots speed unit, got '%c', expected 'N'", pack->spn_n);
      return false;
    }

    speedN = true;
    nmea_INFO_set_present(&pack->present, SPEED);
  } else {
    pack->spn = 0.0;
    pack->spn_n = '\0';
  }

  if (!isnan(pack->spk) && (pack->spk_k)) {
    pack->spk_k = toupper(pack->spk_k);
    if (pack->spk_k != 'K') {
      nmeaError("GPVTG parse error: invalid kph speed unit, got '%c', expected 'K'", pack->spk_k);
      return false;
    }

    speedK = true;
    nmea_INFO_set_present(&pack->present, SPEED);

  }

  if (!speedK && speedN) {
    pack->spk = pack->spn * NMEA_TUD_KNOTS;
    pack->spk_k = 'K';
  } else if (speedK && !speedN) {
    pack->spn = pack->spk / NMEA_TUD_KNOTS;
    pack->spn_n = 'N';
  }

  return true;

  err:
    memset(pack, 0, sizeof(*pack));
    return false;
}

void nmeaGPVTGToInfo(const nmeaGPVTG *pack, nmeaINFO *info) {
  if (!pack || !info) {
    return;
  }

  nmea_INFO_set_present(&info->present, SMASK);

  info->smask |= GPVTG;

  if (nmea_INFO_is_present(pack->present, TRACK)) {
    info->track = pack->track;
    nmea_INFO_set_present(&info->present, TRACK);
  }

  if (nmea_INFO_is_present(pack->present, MTRACK)) {
    info->mtrack = pack->mtrack;
    nmea_INFO_set_present(&info->present, MTRACK);
  }

  if (nmea_INFO_is_present(pack->present, SPEED)) {
    double speed;
    if (pack->spk_k) {
      speed = pack->spk;
    } else {
      speed = pack->spn * NMEA_TUD_KNOTS;
    }
    info->speed = speed;
    nmea_INFO_set_present(&info->present, SPEED);
  }
}

void nmeaGPVTGFromInfo(const nmeaINFO *info, nmeaGPVTG *pack) {
  if (!pack || !info) {
    return;
  }

  memset(pack, 0, sizeof(*pack));

  if (nmea_INFO_is_present(info->present, TRACK)) {
    pack->track = info->track;
    pack->track_t = 'T';
    nmea_INFO_set_present(&pack->present, TRACK);
  }

  if (nmea_INFO_is_present(info->present, MTRACK)) {
    pack->mtrack = info->mtrack;
    pack->mtrack_m = 'M';
    nmea_INFO_set_present(&pack->present, MTRACK);
  }

  if (nmea_INFO_is_present(info->present, SPEED)) {
    pack->spn = info->speed / NMEA_TUD_KNOTS;
    pack->spn_n = 'N';
    pack->spk = info->speed;
    pack->spk_k = 'K';
    nmea_INFO_set_present(&pack->present, SPEED);
  }
}

int nmeaGPVTGgenerate(char *s, const size_t sz, const nmeaGPVTG *pack) {
  char sTrackT[16];
  char sTrackM[16];
  char sSpeedN[16];
  char sSpeedK[16];
  char sUnitT[2];
  char sUnitM[2];
  char sUnitN[2];
  char sUnitK[2];

  sTrackT[0] = 0;
  sTrackM[0] = 0;
  sSpeedN[0] = 0;
  sSpeedK[0] = 0;
  sUnitT[0] = sUnitT[1] = 0;
  sUnitM[0] = sUnitM[1] = 0;
  sUnitN[0] = sUnitN[1] = 0;
  sUnitK[0] = sUnitK[1] = 0;

  if (nmea_INFO_is_present(pack->present, TRACK)) {
    snprintf(&sTrackT[0], sizeof(sTrackT), "%03.1f", pack->track);
    sUnitT[0] = 'T';
  }
  if (nmea_INFO_is_present(pack->present, MTRACK)) {
    snprintf(&sTrackM[0], sizeof(sTrackM), "%03.1f", pack->mtrack);
    sUnitM[0] = 'M';
  }
  if (nmea_INFO_is_present(pack->present, SPEED)) {
    snprintf(&sSpeedN[0], sizeof(sSpeedN), "%03.1f", pack->spn);
    sUnitN[0] = 'N';
    snprintf(&sSpeedK[0], sizeof(sSpeedK), "%03.1f", pack->spk);
    sUnitK[0] = 'K';
  }

  return nmea_printf(s, sz, "$GPVTG,%s,%s,%s,%s,%s,%s,%s,%s", &sTrackT[0], &sUnitT[0], &sTrackM[0], &sUnitM[0],
      &sSpeedN[0], &sUnitN[0], &sSpeedK[0], &sUnitK[0]);
}
