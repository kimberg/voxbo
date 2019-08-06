
// questions re: fiez et al.

// fig 3a shows sizes of 16 and 19 and calculates the percent
// difference as 16%.  the calculation given in the article (diff
// divided by mean size) gives 17%.  which is right?

// fig 3b shows surface distances. the algorithm isn't clear, and not
// spelled out in the refs, so here's my guess.  accumulate all the
// surface voxels for each volume.  surface-surface matches are 0's.
// other surface voxels that don't overlap with non-surface voxels are
// given the distance to the nearest surface voxel (positive for A-B,
// negative for B-A).  calculate euclidean distances, but round.

// vbmaskcompare.cpp
// mask comparison utility for doing inter-rater reliability
// Copyright (c) 2005-2009 by The VoxBo Development Team

// VoxBo is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// VoxBo is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with VoxBo.  If not, see <http://www.gnu.org/licenses/>.
//
// For general information on VoxBo, including the latest complete
// source code and binary distributions, manual, and associated files,
// see the VoxBo home page at: http://www.voxbo.org/
//
// original version written by Dan Kimberg
// based on Fiez et al.

#include <math.h>
#include "vbio.h"
#include "vbmaskcompare.hlp.h"
#include "vbutil.h"
#include "vbversion.h"

class subrect {
 public:
  subrect() { xmin = -1; }
  int32 xmin, xmax, ymin, ymax, zmin, zmax;
  void include(int x, int y, int z);
};

void subrect::include(int x, int y, int z) {
  if (xmin == -1) {
    xmin = xmax = x;
    ymin = ymax = y;
    zmin = zmax = z;
    return;
  }
  if (x < xmin)
    xmin = x;
  else if (x > xmax)
    xmax = x;
  if (y < ymin)
    ymin = y;
  else if (y > ymax)
    ymax = y;
  if (z < zmin)
    zmin = z;
  else if (z > zmax)
    zmax = z;
}

class VBComp {
 private:
  Cube mask1, mask2;
  int m_union, m_intersection, m_just1, m_just2, m_neither;
  int m_discrepant1, m_discrepant2;
  VBRegion surface1, surface2;  // surfaces for the two volumes
  VBRegion unique1, unique2;    // unique voxels for the two volumes
  VBRegion discrepants1, discrepants2, nondiscrepants;
  VBRegion allvoxels1, allvoxels2;
  vector<int> distances;

 public:
  VBComp();
  int surfaceflag;
  // double and integer versions of the maximum allowable non-discrepant
  // distance
  double f_ddist;
  int32 f_idist;
  string stem;
  int load_masks(string m1, string m2);
  void check_volumes();
  void check_volumes2();
  void check_surfaces();
  void check_discrepants();
  void write_discrepants();
  void summarize();
};

void vbmaskcompare_help();
void vbmaskcompare_version();

int main(int argc, char *argv[]) {
  if (argc == 1) {
    vbmaskcompare_help();
    exit(0);
  }
  VBComp cmp;
  cmp.surfaceflag = 0;
  tokenlist args;
  vector<string> filelist;
  args.Transfer(argc - 1, argv + 1);
  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-s")
      cmp.surfaceflag = 1;
    else if (args[i] == "-d")  // OBSOLETED
      continue;
    else if (args[i] == "-dd" && i < args.size() - 1) {
      cmp.f_ddist = strtod(args[i + 1]);
      cmp.f_idist = strtol(args[i + 1]);
      i++;
    } else if (args[i] == "-o" && i < args.size() - 1)
      cmp.stem = args[++i];
    else if (args[i] == "-h") {
      vbmaskcompare_help();
      exit(0);
    } else if (args[i] == "-v") {
      vbmaskcompare_version();
      exit(0);
    } else
      filelist.push_back(args[i]);
  }

  if (filelist.size() != 2) {
    vbmaskcompare_help();
    exit(5);
  }

  if (cmp.load_masks(filelist[0], filelist[1])) {
    printf("[E] vbmaskcompare: couldn't load masks (or discrepant sizes)\n");
    exit(10);
  }
  // printf("[I] vbmaskcompare: checking volumes\n");
  cmp.check_volumes();
  // cmp.check_volumes2();  // little extra work to establish bounding box
  if (cmp.surfaceflag) {
    printf("[I] vbmaskcompare: checking surfaces\n");
    cmp.check_surfaces();
  }
  cmp.check_discrepants();
  cmp.summarize();
  if (cmp.stem.size()) {
    cmp.write_discrepants();
  }
  exit(0);
}

VBComp::VBComp() {
  m_intersection = 0;
  m_union = 0;
  m_just1 = 0;
  m_just2 = 0;
  m_neither = 0;
  f_ddist = 2.0;
  f_idist = 2;
}

int VBComp::load_masks(string m1, string m2) {
  mask1.ReadFile(m1);
  mask2.ReadFile(m2);
  if (!(mask1.data)) return 101;
  if (!(mask2.data)) return 102;
  if (mask1.dimx != mask2.dimx || mask1.dimy != mask2.dimy ||
      mask1.dimz != mask2.dimz)
    return 103;
  return 0;
}

void VBComp::check_volumes2() {
  // bool found1=0,found2=0;
  bool v1, v2;
  subrect r1, r2;
  int ind = 0;
  for (int k = 0; k < mask1.dimz; k++) {
    for (int j = 0; j < mask1.dimy; j++) {
      for (int i = 0; i < mask1.dimx; i++) {
        v1 = mask1.testValue(i, j, k);
        v2 = mask2.testValue(i, j, k);
        if (v1) r1.include(i, j, k);
        if (v2) r2.include(i, j, k);
        ind++;
      }
    }
  }
}

void VBComp::check_volumes() {
  bool v1, v2;
  for (int k = 0; k < mask1.dimz; k++) {
    for (int j = 0; j < mask1.dimy; j++) {
      for (int i = 0; i < mask1.dimx; i++) {
        v1 = (int)mask1.testValue(i, j, k);
        v2 = (int)mask2.testValue(i, j, k);
        if (v1 && v2) {
          m_intersection++;
          nondiscrepants.add(i, j, k, 1);
        } else if (v1) {
          unique1.add(i, j, k, 0.0);
          m_just1++;
        } else if (v2) {
          unique2.add(i, j, k, 0.0);
          m_just2++;
        } else
          m_neither++;
        if (v1) {
          allvoxels1.add(i, j, k, 0.0);
          if (mask1.is_surface(i, j, k)) surface1.add(i, j, k, 0.0);
        }
        if (v2) {
          allvoxels2.add(i, j, k, 0.0);
          if (mask2.is_surface(i, j, k)) surface2.add(i, j, k, 0.0);
        }
      }
    }
  }
  m_union = m_intersection + m_just1 + m_just2;
}

// here's my understanding of the algorithm.  matching surface voxels
// count as zeros.  for unmatched surface voxels, that don't overlap
// with the other mask, find the nearest surface voxel on the other
// surface.

void VBComp::check_surfaces() {
  int mindist;
  // distances for first surface (including matching surface voxels)
  // for (int i=0; i<(int)surface1.size(); i++) {
  for (VI vox1 = surface1.begin(); vox1 != surface1.end(); vox1++) {
    mindist = mask1.dimx + mask1.dimy + mask1.dimz;  // suitably large value
    for (VI vox2 = surface2.begin(); vox2 != surface2.end(); vox2++) {
      int dist = (int)round(voxeldistance(vox1->second, vox2->second));
      if (dist < mindist) mindist = dist;
    }
    if (mindist == 0) distances.push_back(0);
    if (mindist > 0 &&
        !(allvoxels2.contains(vox1->second.x, vox1->second.y, vox1->second.z)))
      distances.push_back(mindist);
  }
  // distances for second surface (excluding matching surface voxels)
  // for (int i=0; i<(int)surface2.size(); i++) {
  for (VI vox2 = surface2.begin(); vox2 != surface2.end(); vox2++) {
    mindist = mask1.dimx + mask1.dimy + mask1.dimz;  // suitably large value
    for (VI vox1 = surface1.begin(); vox1 != surface1.end(); vox1++) {
      int dist = (int)round(voxeldistance(vox1->second, vox2->second));
      if (dist < mindist) mindist = dist;
    }
    if (mindist != 0 &&
        !(allvoxels1.contains(vox2->second.x, vox2->second.y, vox2->second.z)))
      distances.push_back(0 - mindist);
  }
  double meandist = 0;
  for (size_t i = 0; i < distances.size(); i++) meandist += distances[i];
  meandist /= distances.size();
  printf("[I] vbmaskcompare: mean inter-surface distance: %g\n", meandist);
}

void VBComp::check_discrepants() {
  m_discrepant1 = m_discrepant2 = 0;
  discrepants1.clear();
  discrepants2.clear();
  double dist, mindist;
  for (VI vox1 = unique1.begin(); vox1 != unique1.end(); vox1++) {
    mindist = 999999999;
    int x1 = vox1->second.x - f_idist;
    int x2 = vox1->second.x + f_idist;
    int y1 = vox1->second.y - f_idist;
    int y2 = vox1->second.y + f_idist;
    int z1 = vox1->second.z - f_idist;
    int z2 = vox1->second.z + f_idist;
    for (int i = x1; i <= x2; i++) {
      for (int j = y1; j <= y2; j++) {
        for (int k = z1; k <= z2; k++) {
          if (!(mask2.testValue(i, j, k))) continue;
          dist = voxeldistance(i, j, k, vox1->second.x, vox1->second.y,
                               vox1->second.z);
          if (dist < mindist) mindist = dist;
          if (dist <= f_ddist) break;
        }
      }
    }
    if (mindist - f_ddist > DBL_MIN) {
      m_discrepant1++;
      discrepants1.add(vox1->second);
    } else
      nondiscrepants.add(vox1->second);
  }
  for (VI vox2 = unique2.begin(); vox2 != unique2.end(); vox2++) {
    mindist = 999999999;
    int x1 = vox2->second.x - f_idist;
    int x2 = vox2->second.x + f_idist;
    int y1 = vox2->second.y - f_idist;
    int y2 = vox2->second.y + f_idist;
    int z1 = vox2->second.z - f_idist;
    int z2 = vox2->second.z + f_idist;
    for (int i = x1; i <= x2; i++) {
      for (int j = y1; j <= y2; j++) {
        for (int k = z1; k <= z2; k++) {
          if (!(mask1.testValue(i, j, k))) continue;
          dist = voxeldistance(i, j, k, vox2->second.x, vox2->second.y,
                               vox2->second.z);
          if (dist < mindist) mindist = dist;
          if (dist <= f_ddist) break;
        }
      }
    }
    if (mindist - f_ddist > DBL_MIN) {
      m_discrepant2++;
      discrepants2.add(vox2->second);
    } else
      nondiscrepants.add(vox2->second);
  }
}

void VBComp::write_discrepants() {
  Cube cb;
  string fname;
  cb.SetVolume(mask1.dimx, mask1.dimy, mask1.dimz, vb_byte);
  cb.zero();
  for (VI vox = discrepants1.begin(); vox != discrepants1.end(); vox++)
    cb.setValue<char>(vox->second.x, vox->second.y, vox->second.z, 1);
  if (discrepants1.size() == 0)
    cout << format("[W] vbmaskcompare: no discrepant voxels in first mask\n");
  fname = stem;
  replace_string(fname, "XXX", "d1");
  if (cb.WriteFile(fname))
    cout << format("[E] vbmaskcompare: couldn't write output file %s\n") %
                fname;
  else
    cout << format("[I] vbmaskcompare: wrote output file %s\n") % fname;

  cb.zero();
  for (VI vox = discrepants2.begin(); vox != discrepants2.end(); vox++)
    cb.setValue<char>(vox->second.x, vox->second.y, vox->second.z, 1);
  if (discrepants2.size() == 0)
    cout << format("[W] vbmaskcompare: no discrepant voxels in second mask\n");
  fname = stem;
  replace_string(fname, "XXX", "d2");
  if (cb.WriteFile(fname))
    cout << format("[E] vbmaskcompare: couldn't write output file %s\n") %
                fname;
  else
    cout << format("[I] vbmaskcompare: wrote output file %s\n") % fname;

  cb.zero();
  for (VI vox = nondiscrepants.begin(); vox != nondiscrepants.end(); vox++)
    cb.setValue<char>(vox->second.x, vox->second.y, vox->second.z, 1);
  if (nondiscrepants.size() == 0)
    cout << format("[W] vbmaskcompare: no nondiscrepant voxels anywhere\n");
  fname = stem;
  replace_string(fname, "XXX", "nond");
  if (cb.WriteFile(fname))
    cout << format("[E] vbmaskcompare: couldn't write output file %s\n") %
                fname;
  else
    cout << format("[I] vbmaskcompare: wrote output file %s\n") % fname;
}

void VBComp::summarize() {
  if (m_union == 0) {
    printf("[E] vbmaskcompare: no voxels found in either mask\n");
    return;
  }
  printf("[I] vbmaskcompare: total voxels: %d\n", m_union + m_neither);
  printf("[I] vbmaskcompare: %s: %d total, %d unique\n",
         mask1.GetFileName().c_str(), m_intersection + m_just1, m_just1);
  printf("[I] vbmaskcompare: %s: %d total, %d unique\n",
         mask2.GetFileName().c_str(), m_intersection + m_just2, m_just2);
  printf("[I] vbmaskcompare: total mask voxels (mask union): %d\n", m_union);
  printf("[I] vbmaskcompare: common (mask intersection): %d\n", m_intersection);
  float dice = (2.0 * m_intersection) / (m_union + m_intersection);
  printf("[I] vbmaskcompare: dice overlap: %.4f\n", dice);
  float disc = (float)fabs(m_union - m_intersection) / (float)m_union;
  printf("[I] vbmaskcompare: percent non-overlapping: %.2f%%\n", 100.0 * disc);
  printf("[I] vbmaskcompare: discrepant voxels in %s: %d\n",
         mask1.GetFileName().c_str(), m_discrepant1);
  printf("[I] vbmaskcompare: discrepant voxels in %s: %d\n",
         mask2.GetFileName().c_str(), m_discrepant2);
  printf("[I] vbmaskcompare: total discrepant voxels: %d (%.2f%%)\n",
         m_discrepant1 + m_discrepant2,
         (float)(m_discrepant1 + m_discrepant2) / (float)m_union);
}

void vbmaskcompare_help() { cout << boost::format(myhelp) % vbversion; }

void vbmaskcompare_version() {
  printf("VoxBo vbmaskcompare (v%s)\n", vbversion.c_str());
}
