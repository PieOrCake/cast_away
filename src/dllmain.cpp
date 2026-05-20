#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <filesystem>
#include <ctime>
#include <cmath>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include "nexus/Nexus.h"
#include "imgui.h"
#include <nlohmann/json.hpp>
#include "FishData.h"
#include "MapPanel.h"
#include "IconManager.h"
#include "PriceCache.h"
#include "AchievementTracker.h"
#include "FishTank.h"

#define V_MAJOR    0
#define V_MINOR    1
#define V_BUILD    0
#define V_REVISION 0
#define QA_ID          "QA_CAST_AWAY"
#define TEX_ICON       "TEX_CAST_AWAY_ICON"
#define TEX_ICON_HOVER "TEX_CAST_AWAY_ICON_HOVER"

// ---------------------------------------------------------------------------
// Embedded QA icon data (32x32 RGBA PNG, fishing theme)
// ---------------------------------------------------------------------------
static const unsigned char ICON_NORMAL[] = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
  0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20,
  0x08, 0x06, 0x00, 0x00, 0x00, 0x73, 0x7a, 0x7a, 0xf4, 0x00, 0x00, 0x00,
  0x06, 0x62, 0x4b, 0x47, 0x44, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0xa0,
  0xbd, 0xa7, 0x93, 0x00, 0x00, 0x07, 0xf0, 0x49, 0x44, 0x41, 0x54, 0x58,
  0x85, 0xed, 0x56, 0x7b, 0x6c, 0x53, 0xd7, 0x19, 0xff, 0xee, 0xbd, 0x7e,
  0x3f, 0xae, 0xed, 0xf8, 0x11, 0xfc, 0x48, 0xd2, 0x24, 0x4e, 0x9c, 0x90,
  0x84, 0xf0, 0x2a, 0x10, 0x02, 0x69, 0x4a, 0x4b, 0x43, 0x18, 0xa1, 0x41,
  0x1b, 0x59, 0xc7, 0x5a, 0xd6, 0x6d, 0x05, 0x4d, 0xaa, 0x3a, 0x6d, 0x15,
  0x62, 0xda, 0xd6, 0x76, 0x54, 0x30, 0xa6, 0xa9, 0x9b, 0x5a, 0x75, 0x9a,
  0x56, 0x41, 0x57, 0x75, 0xed, 0x34, 0x15, 0x10, 0x1d, 0x6d, 0x02, 0x09,
  0x10, 0x2d, 0x01, 0x12, 0xa0, 0x02, 0x42, 0xc8, 0xc3, 0x49, 0xa8, 0x1d,
  0xc7, 0x4e, 0x62, 0x3b, 0x89, 0xe3, 0xc7, 0xbd, 0x7e, 0xdc, 0x7b, 0x6d,
  0xdf, 0x3b, 0x5d, 0xb7, 0x44, 0xa4, 0x01, 0x4a, 0x2b, 0xb4, 0xee, 0x8f,
  0xfe, 0xa4, 0x73, 0x7c, 0xcf, 0x27, 0xfd, 0xbe, 0xef, 0x77, 0xbe, 0xef,
  0x9c, 0xcf, 0x07, 0xe0, 0x7f, 0x84, 0xbd, 0x3b, 0x57, 0xb5, 0xf3, 0xe3,
  0x8b, 0x76, 0x14, 0xbe, 0x61, 0x08, 0xf8, 0xa9, 0xb6, 0x58, 0xa9, 0x13,
  0x29, 0x55, 0x3b, 0x8a, 0x4b, 0x0a, 0xd7, 0xa4, 0xd3, 0x1c, 0x37, 0xea,
  0x70, 0x5c, 0xa2, 0x09, 0xf2, 0x83, 0xce, 0x9b, 0x64, 0xe0, 0x5e, 0xe4,
  0xaf, 0xcb, 0x9b, 0x27, 0xa0, 0xe9, 0xb1, 0x92, 0xdf, 0x6f, 0x6d, 0x7c,
  0x72, 0xcf, 0xfa, 0x47, 0xd7, 0x8b, 0x26, 0xc7, 0x27, 0x00, 0x41, 0x10,
  0x30, 0x59, 0xcc, 0x3b, 0x2e, 0x74, 0x9c, 0xff, 0xb3, 0xfe, 0xa3, 0xe6,
  0xd7, 0x22, 0xd3, 0xc1, 0x3f, 0x9c, 0xe9, 0x9b, 0x8a, 0xdd, 0x4e, 0x7a,
  0x62, 0x49, 0xb6, 0x5c, 0xad, 0xd7, 0xfc, 0xe6, 0x5e, 0xbc, 0xd1, 0x50,
  0xe4, 0xe0, 0xb5, 0x6b, 0xde, 0xf8, 0x97, 0x09, 0x40, 0xba, 0x4f, 0xfd,
  0x8d, 0xeb, 0xbb, 0xd1, 0x47, 0x9e, 0xfc, 0xe8, 0xd4, 0xbe, 0x10, 0x41,
  0x9e, 0x16, 0x72, 0x1c, 0xa2, 0xc4, 0xf1, 0x4d, 0x9b, 0x1b, 0x37, 0xbf,
  0x52, 0x57, 0xbf, 0x49, 0x79, 0xa6, 0xed, 0x34, 0x79, 0xea, 0xc4, 0xa9,
  0x03, 0xe1, 0x70, 0xa4, 0x95, 0x13, 0x00, 0xa2, 0xc6, 0x55, 0xf5, 0x5b,
  0x9e, 0xfc, 0xce, 0x6f, 0x37, 0xd6, 0x3d, 0xa1, 0x3c, 0xdd, 0xda, 0x76,
  0x57, 0x5e, 0x5b, 0x6b, 0x5b, 0xa4, 0xf9, 0xdf, 0xcd, 0x2f, 0x25, 0xa8,
  0xf8, 0x89, 0x24, 0x8d, 0x46, 0x56, 0x2d, 0xcf, 0x3b, 0x27, 0x14, 0x8b,
  0x0b, 0x88, 0x38, 0xb4, 0xbc, 0x75, 0xf4, 0xe2, 0xd3, 0x73, 0x02, 0xd6,
  0x95, 0x6b, 0xb6, 0xc8, 0x50, 0x51, 0xc7, 0x9d, 0x76, 0xa9, 0x32, 0x64,
  0xfd, 0x7a, 0x6b, 0x63, 0xc3, 0x9e, 0x9a, 0xda, 0x1a, 0xb1, 0x77, 0xd2,
  0x9b, 0xd9, 0xa5, 0xd1, 0x68, 0x82, 0x73, 0xe7, 0x3a, 0xe9, 0xe6, 0x13,
  0x2d, 0x7f, 0xba, 0x5b, 0x76, 0xb2, 0xb2, 0x75, 0x2f, 0x6f, 0xdb, 0xde,
  0xf8, 0xe2, 0x9a, 0xb5, 0x55, 0xc2, 0x91, 0xa1, 0x11, 0xa0, 0x12, 0x14,
  0x38, 0x86, 0x7a, 0x20, 0x99, 0x4a, 0x81, 0x73, 0x2c, 0x70, 0xe4, 0xd0,
  0xb1, 0xee, 0xa7, 0xe6, 0x04, 0x00, 0x00, 0xb2, 0xa1, 0x52, 0xdb, 0x98,
  0x9f, 0x63, 0x68, 0x52, 0xab, 0x14, 0x45, 0xf1, 0x38, 0xed, 0xf3, 0xfa,
  0x82, 0x5d, 0xd3, 0x24, 0x71, 0xf8, 0xd2, 0x20, 0x11, 0x7c, 0xac, 0x44,
  0xa9, 0xc5, 0xe4, 0xd2, 0x26, 0xab, 0xd5, 0x56, 0xcd, 0x13, 0x3e, 0x75,
  0x8c, 0x74, 0x45, 0x29, 0xea, 0x28, 0x86, 0x22, 0x9c, 0x46, 0x81, 0xff,
  0xd4, 0xb8, 0x48, 0xbd, 0x1e, 0x97, 0x4b, 0x2c, 0x64, 0x8c, 0x1a, 0xf7,
  0xfa, 0xc3, 0x5d, 0xa1, 0x28, 0xf1, 0xf7, 0xae, 0xfe, 0x48, 0x68, 0x7d,
  0xa9, 0xce, 0xa8, 0xc0, 0xc5, 0xdf, 0xaf, 0x5c, 0x5a, 0xd9, 0x80, 0x20,
  0xa8, 0x90, 0x21, 0x67, 0x0a, 0x59, 0x2e, 0x3d, 0xf1, 0xfa, 0xbf, 0xae,
  0xae, 0x9e, 0x57, 0x82, 0x3d, 0xcf, 0xac, 0xf4, 0x54, 0x96, 0xe5, 0xe7,
  0x78, 0xfd, 0xc1, 0x44, 0x84, 0x48, 0x4c, 0xc9, 0x64, 0x22, 0xb5, 0xd1,
  0xa0, 0x51, 0x47, 0x63, 0x54, 0xaa, 0xb7, 0x7f, 0xec, 0x43, 0x97, 0xcb,
  0xbf, 0xbf, 0x73, 0x28, 0x34, 0x70, 0x8b, 0x50, 0x53, 0xa2, 0xa9, 0xb0,
  0x16, 0x2c, 0x7a, 0x79, 0x49, 0x79, 0xde, 0x36, 0x5c, 0x21, 0x15, 0xf8,
  0xa6, 0xc3, 0x44, 0x3c, 0x4e, 0x07, 0x64, 0x32, 0xb1, 0xce, 0x68, 0x50,
  0xe3, 0x44, 0x34, 0x91, 0xba, 0x31, 0xe0, 0x5e, 0xc0, 0xfb, 0xe5, 0x0f,
  0x57, 0x5e, 0x06, 0x0e, 0x1e, 0xf2, 0xf9, 0x66, 0xdf, 0xfc, 0xa0, 0xc3,
  0x75, 0x70, 0xee, 0x10, 0x46, 0x08, 0xca, 0xf9, 0xfe, 0x91, 0x8e, 0xbd,
  0x67, 0xae, 0x07, 0x8e, 0x02, 0x00, 0x9b, 0x09, 0x52, 0xae, 0x2d, 0xcd,
  0xcf, 0xd5, 0xed, 0x5b, 0x5e, 0x96, 0xbf, 0x6d, 0x7d, 0x55, 0x69, 0x53,
  0xdd, 0x74, 0x88, 0x8c, 0xc6, 0x99, 0x80, 0x4c, 0x26, 0xd2, 0x1b, 0x0d,
  0x1a, 0x05, 0x41, 0xc6, 0x92, 0xd7, 0x07, 0x5c, 0x1f, 0x8e, 0xba, 0x67,
  0x5e, 0xbd, 0x30, 0x18, 0xb4, 0xdf, 0x72, 0x56, 0xbd, 0x38, 0xab, 0xcc,
  0xfa, 0x90, 0xfe, 0x95, 0x3b, 0xf1, 0x92, 0x4c, 0x5a, 0x91, 0x4a, 0xa7,
  0x38, 0x60, 0x01, 0x9b, 0x97, 0x01, 0xb8, 0x07, 0x6a, 0x97, 0xaa, 0xd5,
  0x72, 0x81, 0xfc, 0x59, 0x8b, 0x59, 0x53, 0xa3, 0x50, 0x48, 0x73, 0xa3,
  0xb1, 0x84, 0x7b, 0x62, 0x22, 0x74, 0x21, 0x96, 0x8a, 0xbd, 0xdb, 0xd9,
  0x1b, 0x0e, 0x7f, 0x15, 0x9e, 0x54, 0x22, 0x2e, 0x17, 0x0a, 0x51, 0xdf,
  0x6b, 0xef, 0x5f, 0xa9, 0x85, 0x6f, 0x02, 0x7b, 0xff, 0xaf, 0x3b, 0xe1,
  0xd7, 0x45, 0xb5, 0x4d, 0x69, 0x43, 0x51, 0xc1, 0x06, 0x4c, 0x00, 0x8b,
  0x65, 0x62, 0x91, 0x3a, 0x4e, 0x31, 0xee, 0x74, 0x9a, 0x1d, 0x43, 0x30,
  0xcc, 0x7e, 0x7e, 0x20, 0x78, 0xf9, 0xd6, 0x99, 0xe2, 0xc1, 0xd0, 0x29,
  0xd1, 0x9d, 0x7c, 0x08, 0xea, 0x57, 0x67, 0xe1, 0xad, 0x9f, 0x04, 0x89,
  0xaf, 0x12, 0xb8, 0xd6, 0xa6, 0x5a, 0x69, 0x34, 0x6b, 0xde, 0x59, 0x6c,
  0x35, 0x56, 0x58, 0x4c, 0x3a, 0xc0, 0x30, 0x04, 0x58, 0x0e, 0x00, 0x45,
  0x00, 0x58, 0x16, 0x20, 0x4c, 0x44, 0xa1, 0xcc, 0xea, 0x8b, 0xfa, 0xa6,
  0xc8, 0xe6, 0x00, 0x11, 0xfd, 0x63, 0xd7, 0x60, 0xf8, 0x86, 0xca, 0x90,
  0x57, 0x91, 0x4a, 0x31, 0xec, 0x02, 0x01, 0x16, 0xbd, 0xf6, 0xe3, 0x4d,
  0xcb, 0x05, 0x6f, 0xb7, 0xf5, 0x4c, 0xff, 0xf3, 0x7e, 0x82, 0x57, 0x15,
  0x6b, 0xcd, 0xd6, 0x42, 0xed, 0x99, 0xaa, 0x87, 0x8b, 0x35, 0x81, 0x59,
  0x82, 0xed, 0x1f, 0x1a, 0x03, 0x84, 0x43, 0x10, 0xbe, 0x4b, 0xb1, 0x1c,
  0xc7, 0x01, 0x70, 0x88, 0x52, 0x29, 0x61, 0x2b, 0x4a, 0x72, 0xe5, 0xab,
  0x96, 0x8a, 0x7e, 0xd0, 0x7d, 0x75, 0xe4, 0x91, 0xae, 0xc1, 0xb0, 0x39,
  0x27, 0x2f, 0x97, 0x73, 0xbb, 0x3d, 0xbd, 0x0b, 0x04, 0xd0, 0x6c, 0x7a,
  0xb7, 0x5e, 0x27, 0x6f, 0x7d, 0xb6, 0xbe, 0x50, 0xfa, 0x6e, 0xab, 0xf3,
  0xf0, 0x5d, 0xe2, 0x22, 0xeb, 0x4a, 0xd5, 0x8d, 0xa5, 0x45, 0xe6, 0xdf,
  0x95, 0x97, 0xe6, 0x54, 0x66, 0x67, 0x67, 0x83, 0x42, 0x21, 0x87, 0xd5,
  0x6b, 0xcd, 0x68, 0x3a, 0x9d, 0x84, 0x14, 0x43, 0x43, 0x92, 0x8e, 0xdd,
  0x7e, 0xab, 0x50, 0x8e, 0xe5, 0xa0, 0x7f, 0xd8, 0x03, 0xb9, 0x39, 0x46,
  0xd3, 0x73, 0x5b, 0xb1, 0xbe, 0x28, 0x41, 0xaa, 0xfd, 0x93, 0xbe, 0xc1,
  0x05, 0x8e, 0x01, 0x00, 0x7e, 0xb6, 0xb5, 0xe4, 0xec, 0xc4, 0x54, 0xa4,
  0x25, 0xdf, 0xa2, 0xa9, 0x75, 0xc4, 0xed, 0x4f, 0xb5, 0xb6, 0x02, 0x3d,
  0x97, 0xee, 0x32, 0xdc, 0x9a, 0x9f, 0x67, 0x3e, 0xba, 0xf9, 0xf1, 0x65,
  0xcb, 0xac, 0xd6, 0x02, 0xc0, 0x75, 0x16, 0x10, 0x08, 0x25, 0x0b, 0x14,
  0xa6, 0x92, 0x14, 0x44, 0x66, 0x27, 0x80, 0x89, 0xdf, 0xb9, 0x9a, 0x9e,
  0xc9, 0x19, 0x38, 0x79, 0xf6, 0xda, 0xd0, 0xa7, 0xe3, 0x53, 0x0d, 0x17,
  0x07, 0x22, 0xce, 0x5b, 0x76, 0x8c, 0x9f, 0xea, 0x56, 0x59, 0xb4, 0x1e,
  0x6f, 0x78, 0x6d, 0x8e, 0x71, 0xd1, 0x5f, 0x0b, 0x74, 0x39, 0x6d, 0x45,
  0x79, 0xa2, 0xb6, 0x9e, 0xe1, 0xd0, 0x2c, 0xdf, 0xf5, 0xaa, 0xd7, 0x96,
  0x5d, 0xd9, 0xb1, 0x6d, 0x5d, 0x9e, 0xce, 0xa0, 0x03, 0xad, 0xa9, 0x18,
  0x10, 0x54, 0x00, 0xa3, 0x37, 0x87, 0x61, 0xd2, 0xe3, 0x86, 0x69, 0x9f,
  0x37, 0x33, 0xa2, 0x44, 0x04, 0xb4, 0xfa, 0x45, 0x20, 0x53, 0x66, 0x01,
  0x1d, 0x27, 0x80, 0x4d, 0x27, 0xe7, 0x02, 0x27, 0x28, 0x06, 0xae, 0xdd,
  0x70, 0x02, 0x19, 0xa5, 0x60, 0x53, 0xed, 0x32, 0x7d, 0x3a, 0xc5, 0xfe,
  0x58, 0x40, 0xc7, 0x9a, 0xc7, 0x02, 0xd4, 0xf4, 0x9c, 0x80, 0x86, 0xaa,
  0xbc, 0xd9, 0x60, 0x94, 0x7a, 0xe1, 0x70, 0x8b, 0x7d, 0xef, 0xba, 0x72,
  0x8b, 0xbd, 0xc0, 0x6c, 0x68, 0x2f, 0xb6, 0x28, 0x3c, 0x7a, 0x83, 0xe6,
  0x1f, 0xdb, 0x1b, 0xaa, 0x74, 0x08, 0x8a, 0x80, 0x54, 0xa9, 0x05, 0xb1,
  0x14, 0x07, 0xbf, 0x77, 0x02, 0x70, 0x95, 0x1a, 0xf2, 0x0a, 0xac, 0x90,
  0x6d, 0x34, 0x65, 0x06, 0x8a, 0xa2, 0x10, 0x0e, 0xcd, 0x82, 0x12, 0x57,
  0x41, 0x2a, 0x45, 0x41, 0x92, 0x8a, 0xcd, 0x05, 0x7f, 0xe3, 0x50, 0x0b,
  0xf4, 0xdb, 0xc7, 0xc1, 0x52, 0xb8, 0x18, 0xce, 0x76, 0x5c, 0x81, 0xea,
  0x95, 0x45, 0xe2, 0x10, 0x19, 0xdb, 0x8e, 0x2a, 0x03, 0x6f, 0xf8, 0x7c,
  0xc0, 0x66, 0x04, 0x54, 0x17, 0xa8, 0xa3, 0x71, 0x96, 0x7b, 0x0e, 0x53,
  0x49, 0xdf, 0x3b, 0xd2, 0x36, 0x34, 0xb2, 0xa2, 0x28, 0xfb, 0xaa, 0x58,
  0x22, 0x3a, 0xbe, 0xe6, 0xe1, 0x62, 0x95, 0x0a, 0x97, 0x7d, 0xb6, 0x15,
  0x8e, 0x05, 0x09, 0xae, 0x05, 0x1c, 0x57, 0x83, 0x44, 0xfa, 0xb9, 0x8d,
  0x37, 0x73, 0x1c, 0x78, 0xc7, 0xdd, 0x10, 0x09, 0x85, 0x60, 0xca, 0x37,
  0x09, 0xfe, 0x71, 0x37, 0xc4, 0x63, 0x71, 0x50, 0x28, 0x65, 0x70, 0xb5,
  0x77, 0x14, 0x7a, 0xfa, 0x46, 0x21, 0xbf, 0x30, 0x1f, 0xf6, 0x1f, 0xdc,
  0x0f, 0x0e, 0xa7, 0x0b, 0xa6, 0xbc, 0xe3, 0xc0, 0x25, 0x19, 0x79, 0xc0,
  0xcf, 0xf4, 0x79, 0x66, 0x68, 0x7b, 0xa6, 0x11, 0xed, 0x3b, 0x36, 0xc8,
  0x60, 0x28, 0x4a, 0x49, 0xa2, 0x8c, 0x9a, 0x5f, 0x1f, 0x78, 0xef, 0x72,
  0xfb, 0x4d, 0x97, 0xff, 0x3f, 0xd9, 0xba, 0xcc, 0x32, 0x83, 0x24, 0x9d,
  0x80, 0x90, 0xd7, 0x91, 0xa9, 0xf5, 0xed, 0xe0, 0x33, 0xa2, 0xd5, 0x1b,
  0xa0, 0xb4, 0xa2, 0x1c, 0xcc, 0x8b, 0x54, 0x90, 0x93, 0x63, 0x00, 0x5c,
  0x25, 0x87, 0x70, 0x90, 0x00, 0xe4, 0xf3, 0x23, 0x39, 0xea, 0x18, 0x85,
  0xe7, 0x77, 0x3f, 0x0f, 0xed, 0xa7, 0xdb, 0x41, 0x97, 0xa5, 0x02, 0x91,
  0x58, 0x08, 0xc0, 0x22, 0x6b, 0x33, 0xb7, 0x80, 0x9f, 0xde, 0x7c, 0xa1,
  0x5e, 0xdc, 0x33, 0x32, 0x22, 0x65, 0x63, 0x92, 0x10, 0xbf, 0x7e, 0x69,
  0xe7, 0x9a, 0xc7, 0x11, 0x01, 0x6c, 0x98, 0x0a, 0x84, 0x21, 0xc7, 0xa4,
  0x9b, 0x0b, 0xc6, 0x50, 0x51, 0x98, 0x1e, 0x1f, 0x84, 0x99, 0x19, 0x02,
  0x92, 0x4c, 0x1a, 0x10, 0x14, 0x03, 0x8e, 0x4d, 0x83, 0x50, 0x84, 0x81,
  0x5e, 0x8f, 0x03, 0x9b, 0x62, 0xe1, 0xec, 0xb9, 0x5e, 0xa8, 0x5d, 0xb7,
  0x04, 0x34, 0x72, 0x19, 0x54, 0xca, 0xa4, 0xd0, 0xd9, 0x3d, 0x08, 0xb3,
  0x21, 0x12, 0x9c, 0x0e, 0x27, 0x68, 0x35, 0x4a, 0x50, 0xca, 0x25, 0x40,
  0x33, 0x49, 0x48, 0xa3, 0x5c, 0x77, 0xe6, 0xba, 0x64, 0x76, 0xc7, 0x92,
  0x79, 0xb1, 0x44, 0x12, 0xbd, 0x34, 0x31, 0x91, 0xe0, 0x83, 0x1b, 0x74,
  0xf2, 0xe3, 0x11, 0x82, 0x7e, 0xfa, 0xdc, 0xc5, 0x01, 0x0f, 0x7f, 0xb3,
  0x6f, 0x07, 0xbf, 0x33, 0xb9, 0x4c, 0x08, 0x66, 0x73, 0x16, 0x98, 0x8c,
  0xaa, 0xcc, 0x2f, 0xbf, 0xe6, 0xed, 0x19, 0x5f, 0x89, 0x04, 0xb4, 0xb4,
  0x7d, 0x92, 0xf9, 0x96, 0x4a, 0x44, 0xf0, 0x8b, 0xdd, 0x5b, 0xe0, 0x7b,
  0x0d, 0x6b, 0x32, 0x83, 0xff, 0xe6, 0x4b, 0x43, 0x26, 0xd2, 0x33, 0xb4,
  0x24, 0xf2, 0xf1, 0xdc, 0x21, 0x5c, 0x61, 0xd3, 0xfc, 0xc8, 0x35, 0x19,
  0x14, 0x7e, 0x77, 0x43, 0xc5, 0xac, 0x44, 0x8a, 0x1c, 0xba, 0xe9, 0xf0,
  0xd6, 0xbc, 0xd5, 0x6c, 0x3f, 0x2f, 0xc3, 0x52, 0x6d, 0x71, 0x8a, 0x7a,
  0xc6, 0x56, 0x60, 0x92, 0xf0, 0xaf, 0xa1, 0x8c, 0x53, 0xa9, 0x04, 0x44,
  0x22, 0xe1, 0x3c, 0x51, 0xfc, 0x9a, 0xb7, 0xf3, 0x87, 0xd1, 0xe5, 0x9c,
  0x04, 0x89, 0x08, 0x03, 0x3a, 0x05, 0xa0, 0xd7, 0xe1, 0x20, 0x10, 0x60,
  0x60, 0x31, 0x6a, 0x33, 0x03, 0x43, 0x51, 0xe8, 0xe8, 0x1e, 0x20, 0x07,
  0x06, 0xdc, 0x35, 0x97, 0xfa, 0x28, 0xff, 0x5c, 0x09, 0x42, 0x11, 0x6a,
  0xb3, 0x48, 0x28, 0x3a, 0x91, 0xa0, 0xc9, 0x9f, 0xf7, 0x87, 0xec, 0x65,
  0xad, 0xa7, 0x3f, 0xeb, 0x03, 0x9d, 0xfd, 0xb3, 0xc3, 0x29, 0x48, 0x2d,
  0xf7, 0xfb, 0xc2, 0xc7, 0xea, 0x37, 0x2e, 0x5b, 0x9e, 0x6b, 0xd6, 0xc3,
  0x97, 0x01, 0x13, 0x22, 0x80, 0xa1, 0x18, 0xdc, 0x18, 0x18, 0x85, 0xc5,
  0x36, 0xcb, 0xfc, 0x3e, 0xd0, 0x7e, 0xfd, 0xaa, 0xcb, 0xe9, 0x6b, 0x3a,
  0x3f, 0x12, 0x71, 0xdd, 0xb2, 0x0b, 0x7e, 0xb2, 0x29, 0xdf, 0x16, 0xa3,
  0x99, 0x7c, 0x9d, 0x56, 0x4a, 0xff, 0xe5, 0xb8, 0x7d, 0xdb, 0x17, 0x1d,
  0x76, 0xf5, 0x47, 0x46, 0xbb, 0xfa, 0x23, 0x2b, 0x1d, 0x6e, 0xff, 0x56,
  0x5b, 0x91, 0x69, 0x5f, 0xb9, 0x2d, 0x77, 0xa9, 0xad, 0xd0, 0x0c, 0x1a,
  0xb5, 0x7c, 0x41, 0x70, 0x26, 0x95, 0x04, 0x91, 0x50, 0xc0, 0x5f, 0x0d,
  0x20, 0xa3, 0x71, 0x08, 0x85, 0x63, 0x30, 0xec, 0x98, 0x84, 0xc1, 0x61,
  0xcf, 0xf5, 0x11, 0xa7, 0xf7, 0xd5, 0x0b, 0xf6, 0x30, 0x9f, 0xf6, 0x79,
  0x45, 0x45, 0x76, 0x6d, 0x29, 0xea, 0xf4, 0x7a, 0xa3, 0x6f, 0x9f, 0xec,
  0xf1, 0xdd, 0xd7, 0x7f, 0x01, 0xff, 0xd6, 0x43, 0x20, 0xb9, 0xd9, 0xa0,
  0xcf, 0x7a, 0x54, 0xaf, 0x53, 0x56, 0xe2, 0x4a, 0x99, 0x49, 0x24, 0x12,
  0xc8, 0x10, 0x00, 0x49, 0x92, 0xa1, 0x19, 0x8d, 0x5c, 0x28, 0xa2, 0x99,
  0x34, 0x5c, 0xec, 0x1d, 0x0b, 0x90, 0x71, 0xfa, 0x57, 0x02, 0x44, 0x78,
  0xaa, 0x73, 0x70, 0x26, 0x93, 0xee, 0x3b, 0xa2, 0x7e, 0x75, 0x16, 0x0e,
  0x0f, 0x08, 0x2f, 0x36, 0x55, 0xbc, 0x7e, 0x60, 0xd7, 0x2a, 0x6e, 0x67,
  0x5d, 0x21, 0xfb, 0x48, 0xa9, 0x6a, 0xe3, 0xfd, 0x70, 0x90, 0x07, 0x15,
  0xbc, 0xde, 0x0a, 0xe2, 0xb2, 0x15, 0x4b, 0xfc, 0x1e, 0x6f, 0x48, 0xe9,
  0x9d, 0x25, 0x77, 0x77, 0xd9, 0xc3, 0xef, 0xdc, 0x0f, 0x0f, 0x7b, 0x50,
  0x02, 0x8a, 0xb2, 0x40, 0xe0, 0x0a, 0x32, 0x38, 0x41, 0xc6, 0x77, 0x75,
  0x0f, 0x13, 0x0b, 0x9e, 0x5e, 0xdf, 0x02, 0xee, 0x82, 0xff, 0x02, 0x06,
  0xf0, 0xc0, 0x4e, 0xea, 0xd5, 0xe8, 0xf5, 0x00, 0x00, 0x00, 0x00, 0x49,
  0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
};
static const unsigned int ICON_NORMAL_size = 2107;

static const unsigned char ICON_HOVER[] = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
  0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20,
  0x08, 0x06, 0x00, 0x00, 0x00, 0x73, 0x7a, 0x7a, 0xf4, 0x00, 0x00, 0x07,
  0xa5, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0xed, 0x56, 0x69, 0x54, 0x94,
  0xd7, 0x19, 0x7e, 0xbe, 0x59, 0x18, 0x66, 0x61, 0x66, 0x18, 0x18, 0x90,
  0x45, 0x51, 0x31, 0x20, 0xa0, 0x10, 0x40, 0x11, 0x54, 0x04, 0x31, 0x8a,
  0x1b, 0xb8, 0xd3, 0x68, 0xf5, 0x68, 0x8d, 0x87, 0xda, 0xa6, 0x31, 0xcd,
  0x69, 0x4f, 0xf4, 0x24, 0x8d, 0x31, 0x6a, 0xac, 0x9e, 0x26, 0xb1, 0x49,
  0x73, 0xaa, 0x45, 0xa4, 0x31, 0x96, 0x62, 0x10, 0x35, 0x71, 0x89, 0xc1,
  0x12, 0x0c, 0x38, 0x22, 0xa2, 0x30, 0xc8, 0x2a, 0xca, 0xa0, 0x20, 0xb2,
  0xcd, 0xc6, 0x0c, 0xcc, 0xc6, 0x2c, 0xb7, 0xe7, 0x1b, 0x64, 0x2a, 0xa8,
  0x89, 0xf1, 0xb4, 0x4d, 0x7e, 0xe4, 0x39, 0xe7, 0x7e, 0x73, 0xef, 0x3b,
  0xf7, 0xb9, 0xef, 0x73, 0xdf, 0xf7, 0x7e, 0xef, 0x77, 0x81, 0xff, 0x13,
  0x72, 0x77, 0x2d, 0x2a, 0xa2, 0xdb, 0x48, 0x3b, 0x03, 0x3f, 0x30, 0x58,
  0xf4, 0x63, 0xf5, 0x54, 0x89, 0x37, 0x57, 0x22, 0x5d, 0x33, 0x35, 0x3e,
  0x3a, 0xde, 0x66, 0x73, 0x90, 0xea, 0xca, 0xaa, 0x2b, 0x46, 0xb5, 0xe6,
  0x58, 0xde, 0x35, 0x8d, 0xea, 0xdb, 0xc8, 0xcf, 0xca, 0x1b, 0x86, 0x6d,
  0xeb, 0xe2, 0xdf, 0xbd, 0x5f, 0xf9, 0x4f, 0x0b, 0x21, 0x36, 0x42, 0x48,
  0x1d, 0x21, 0xa4, 0x81, 0xd0, 0xfd, 0xfb, 0x95, 0xb9, 0x96, 0xad, 0xeb,
  0x13, 0x76, 0x6f, 0x4c, 0x1e, 0xcb, 0x1f, 0xc9, 0xa1, 0x6d, 0xdf, 0xc5,
  0x4b, 0x4d, 0x9d, 0xc0, 0x7b, 0x9a, 0x14, 0x50, 0xc4, 0x51, 0x4b, 0xea,
  0x2e, 0x7e, 0xd3, 0x77, 0xe0, 0xc3, 0x83, 0x3b, 0xba, 0x54, 0x9a, 0x42,
  0x0e, 0x71, 0x50, 0x12, 0x6f, 0xef, 0xf9, 0x9b, 0x7f, 0xbb, 0x79, 0xfb,
  0xa4, 0x94, 0x4d, 0x1e, 0xf5, 0x17, 0x0f, 0xf7, 0xfd, 0xed, 0xcf, 0x07,
  0x77, 0x77, 0xf7, 0x28, 0xcf, 0x13, 0x36, 0x28, 0x1f, 0x6f, 0xe9, 0x82,
  0x97, 0xb7, 0xfc, 0xea, 0xcd, 0xb0, 0xe4, 0x8d, 0x1e, 0x75, 0xc5, 0xd9,
  0x4f, 0xe4, 0xd5, 0x16, 0x67, 0xeb, 0x3e, 0xfe, 0xe0, 0xe3, 0x3f, 0xf4,
  0x19, 0xf4, 0x9f, 0x5b, 0x8c, 0x4c, 0xdd, 0xe2, 0x79, 0x11, 0x25, 0x1c,
  0x1e, 0x6f, 0xbc, 0x4a, 0x8f, 0xb3, 0xaf, 0xee, 0x3d, 0xb5, 0xd6, 0xa5,
  0x60, 0x65, 0xa2, 0xef, 0xe2, 0x27, 0xed, 0x92, 0xde, 0x49, 0xbb, 0x3c,
  0xd7, 0x3c, 0xb8, 0xcb, 0x46, 0x42, 0x48, 0x13, 0x21, 0xc4, 0x4e, 0xda,
  0xe4, 0x47, 0xcd, 0xdf, 0x16, 0x9d, 0x37, 0x37, 0xcc, 0xdc, 0xab, 0x6a,
  0x2c, 0x18, 0x70, 0xf2, 0xb4, 0x97, 0x89, 0xa5, 0xed, 0x02, 0x29, 0xfb,
  0xec, 0xf7, 0xe4, 0xe2, 0xd1, 0x2d, 0xe4, 0x83, 0xad, 0xcb, 0x8f, 0x0d,
  0x8b, 0x00, 0x00, 0x6a, 0xed, 0x6c, 0xff, 0xa5, 0x91, 0x61, 0x41, 0x19,
  0xbe, 0x52, 0xcf, 0xe7, 0xf4, 0x7a, 0x63, 0xe7, 0x6d, 0x45, 0xa7, 0xac,
  0x55, 0xa3, 0x3a, 0xf4, 0x85, 0x4c, 0xad, 0x59, 0x17, 0x2f, 0xf1, 0x62,
  0x8b, 0x04, 0x19, 0x31, 0x31, 0xd3, 0x66, 0xd0, 0x84, 0xeb, 0x55, 0x57,
  0x65, 0x5a, 0x83, 0x21, 0x9f, 0xcd, 0x64, 0x10, 0x5f, 0xb1, 0xd7, 0x4b,
  0xc1, 0xe3, 0x7d, 0x13, 0xbd, 0x45, 0xfc, 0x40, 0x8d, 0xce, 0x70, 0xef,
  0x76, 0x4b, 0xb7, 0xac, 0xbb, 0x57, 0x7d, 0xb8, 0xa0, 0x44, 0xa9, 0x5d,
  0x95, 0x10, 0xe8, 0xe7, 0xe9, 0xcd, 0xfd, 0x59, 0xca, 0x9c, 0x94, 0x34,
  0x8a, 0x62, 0xb2, 0xcd, 0x9a, 0x7b, 0xc1, 0x76, 0x62, 0x6d, 0xdf, 0xb8,
  0xf3, 0xab, 0x69, 0xc3, 0x04, 0x7c, 0xba, 0x73, 0x7e, 0x5b, 0xca, 0xcc,
  0xa8, 0xd1, 0xb7, 0x5b, 0x3a, 0x4c, 0x4a, 0x75, 0x7f, 0xb7, 0x50, 0xe8,
  0x2e, 0x0e, 0x0e, 0x1a, 0x25, 0xee, 0xed, 0x35, 0xd8, 0x8a, 0x4a, 0x6a,
  0x4f, 0xd6, 0xd4, 0xb4, 0xec, 0xca, 0xbb, 0xd2, 0x5d, 0x37, 0x44, 0xc8,
  0x88, 0xf7, 0x9d, 0x1c, 0x1b, 0x35, 0xfe, 0xad, 0xe4, 0x59, 0x11, 0xcb,
  0xbc, 0x3d, 0x3d, 0x58, 0x8a, 0xd6, 0x1e, 0xbd, 0x5e, 0x6f, 0x54, 0x09,
  0x85, 0x3c, 0xef, 0xe0, 0x20, 0x1f, 0xa1, 0x4a, 0xdb, 0x67, 0x2b, 0x2e,
  0xad, 0x7f, 0x84, 0x97, 0xf3, 0xf6, 0xfc, 0x72, 0x10, 0x8c, 0x55, 0x28,
  0x3a, 0x3e, 0x7a, 0x37, 0xb7, 0x66, 0xcf, 0x90, 0x9d, 0xa5, 0x54, 0x19,
  0x14, 0xdb, 0xf7, 0xe4, 0xbe, 0x9e, 0x53, 0xd4, 0x9e, 0x0f, 0xc0, 0xe1,
  0x74, 0x92, 0xe8, 0x1f, 0x16, 0x15, 0x1e, 0xb8, 0x63, 0xde, 0xcc, 0xc8,
  0x65, 0x19, 0x4b, 0x13, 0x32, 0x36, 0xb5, 0x76, 0xf7, 0x69, 0xf5, 0x26,
  0x95, 0x50, 0xc8, 0x95, 0x06, 0x07, 0xf9, 0x0a, 0x54, 0x1a, 0x9d, 0xb5,
  0xa8, 0xb4, 0xe6, 0x64, 0x75, 0xfd, 0xbd, 0x77, 0x8e, 0xcb, 0x3a, 0x1b,
  0x86, 0x16, 0x5b, 0x3e, 0xdd, 0x2f, 0x22, 0x76, 0xf2, 0xe8, 0xed, 0x8f,
  0xe3, 0x59, 0xcc, 0x56, 0xc1, 0x80, 0xcd, 0x4a, 0x60, 0x07, 0x73, 0x64,
  0x0a, 0x9e, 0x88, 0xd5, 0x73, 0x7c, 0xc4, 0x62, 0xb6, 0x68, 0x43, 0x68,
  0xc8, 0xa8, 0x59, 0x9e, 0x9e, 0x1e, 0x63, 0xb4, 0xba, 0xbe, 0xd6, 0xa6,
  0x9b, 0x5d, 0x97, 0x7a, 0xad, 0xba, 0x4f, 0xf2, 0xbe, 0xee, 0xe9, 0xfd,
  0x3e, 0x3c, 0x01, 0x9f, 0x37, 0x89, 0xc3, 0x61, 0x76, 0xae, 0xdb, 0xfe,
  0x65, 0x32, 0x7e, 0x08, 0xe4, 0xfe, 0xa8, 0x2b, 0xe1, 0xb3, 0x62, 0x79,
  0x9c, 0x24, 0x94, 0xc9, 0x64, 0xa7, 0xb0, 0xdc, 0x10, 0x2e, 0xe4, 0x72,
  0xc5, 0x7a, 0x83, 0xa9, 0xd5, 0x66, 0x73, 0xdc, 0xa5, 0x58, 0xac, 0x86,
  0xfc, 0x4b, 0x9d, 0xe5, 0x43, 0x67, 0x8a, 0x86, 0xc9, 0x68, 0x75, 0x7b,
  0xdc, 0x1a, 0xac, 0xcc, 0x34, 0x3f, 0x61, 0xd6, 0x99, 0x4e, 0xfd, 0xf7,
  0x71, 0xbc, 0x3a, 0x4e, 0x3a, 0x25, 0x38, 0xc4, 0x37, 0x67, 0x46, 0x4c,
  0xf0, 0xe4, 0xd0, 0xe7, 0x02, 0xc1, 0x62, 0x31, 0xe0, 0x20, 0x00, 0x83,
  0x02, 0x1c, 0x76, 0xa0, 0x5b, 0xad, 0x45, 0x62, 0xac, 0xa2, 0x5f, 0x71,
  0x47, 0x73, 0xa6, 0x5d, 0xad, 0xdd, 0x57, 0x20, 0xeb, 0xb9, 0x21, 0x0d,
  0x8a, 0x98, 0x6c, 0xb5, 0x9a, 0x1d, 0x8f, 0x08, 0x08, 0x1d, 0xed, 0x7f,
  0x7a, 0xd3, 0x3c, 0x76, 0x76, 0xf6, 0x85, 0xb6, 0x7f, 0x3c, 0x8d, 0xf3,
  0x25, 0x53, 0xfd, 0x03, 0x62, 0xa3, 0x03, 0x2e, 0x2c, 0x59, 0x38, 0xd5,
  0xb3, 0xbd, 0x43, 0xe5, 0x28, 0x29, 0xab, 0x03, 0x83, 0x50, 0x14, 0x28,
  0x8a, 0xb2, 0x13, 0x42, 0x00, 0x42, 0x49, 0x24, 0x7c, 0x47, 0x52, 0x7c,
  0x38, 0x7f, 0xd1, 0x1c, 0xf7, 0xd5, 0x27, 0xcf, 0x57, 0x24, 0x15, 0xc8,
  0x7a, 0x02, 0xc2, 0x26, 0x85, 0x93, 0xfa, 0xba, 0x86, 0xea, 0x47, 0x04,
  0x18, 0xed, 0xb6, 0xcc, 0x31, 0x81, 0xe2, 0xf3, 0x7b, 0x32, 0xbd, 0xb8,
  0x6f, 0x64, 0xc9, 0x0f, 0x3d, 0xc1, 0x2f, 0xb5, 0x32, 0xc1, 0x67, 0x69,
  0xc2, 0x94, 0x90, 0xb7, 0x67, 0x25, 0x4c, 0x8c, 0x1a, 0x3b, 0x76, 0x1c,
  0x3c, 0x3d, 0x45, 0x98, 0x90, 0x1e, 0xc2, 0x00, 0xb1, 0x00, 0x66, 0x23,
  0x60, 0xd4, 0x39, 0xe7, 0x3d, 0x98, 0xcf, 0x80, 0xdd, 0x81, 0x9b, 0xe5,
  0x0d, 0x08, 0x0f, 0x0b, 0xf6, 0x7f, 0xef, 0x15, 0x56, 0x8d, 0x56, 0xa5,
  0x11, 0xb7, 0xdc, 0x52, 0xd4, 0x3f, 0xb2, 0x30, 0x00, 0x7c, 0xf8, 0x4a,
  0xfc, 0xbf, 0x9a, 0xee, 0x2a, 0xcf, 0x46, 0x86, 0xfa, 0x26, 0x57, 0xe9,
  0xcb, 0x5e, 0xcc, 0xca, 0x82, 0xc5, 0x15, 0xee, 0x99, 0x5e, 0x13, 0x22,
  0x23, 0x42, 0xf2, 0x37, 0xaf, 0x9f, 0x1b, 0x2d, 0x8e, 0x8d, 0x02, 0xdc,
  0x42, 0x01, 0x08, 0x1e, 0xa3, 0xb1, 0x1f, 0xb0, 0x36, 0x01, 0x7a, 0xf5,
  0x63, 0x77, 0xa0, 0xbd, 0xd5, 0x86, 0x03, 0x9f, 0x14, 0x36, 0x5e, 0x6f,
  0xbc, 0x9b, 0x76, 0xea, 0x92, 0x52, 0xe1, 0x8a, 0x00, 0x00, 0x04, 0x48,
  0xf9, 0xe7, 0xe4, 0x0d, 0xed, 0xd3, 0x3d, 0x78, 0x5e, 0x1f, 0xa5, 0xfb,
  0xa5, 0xd7, 0x87, 0xbe, 0xd6, 0xba, 0xf0, 0x77, 0xfb, 0x6f, 0xdc, 0xa2,
  0xab, 0xde, 0xb2, 0xf4, 0xe9, 0x65, 0xab, 0xd6, 0xa7, 0x0a, 0xe0, 0x21,
  0x04, 0xdc, 0xa2, 0x07, 0x59, 0x1d, 0x65, 0xb0, 0xeb, 0x9c, 0x3b, 0x76,
  0x82, 0x29, 0x12, 0x01, 0xfe, 0xf1, 0x00, 0x3b, 0x1a, 0xe0, 0x55, 0x00,
  0xc6, 0x87, 0x8e, 0x94, 0xc1, 0x84, 0x0a, 0x59, 0xad, 0xb3, 0xfb, 0xc6,
  0xb6, 0xb5, 0x61, 0xf9, 0x9f, 0x15, 0xcb, 0xdd, 0x6d, 0xe5, 0xd3, 0x87,
  0xaa, 0x24, 0x83, 0x7e, 0x48, 0x85, 0xbc, 0x73, 0xbe, 0x52, 0x41, 0xc4,
  0xcf, 0x77, 0x9e, 0xb9, 0x68, 0x31, 0x62, 0x73, 0x52, 0x64, 0xe4, 0xb5,
  0x43, 0x5b, 0x93, 0xd6, 0x24, 0x25, 0x4e, 0x3a, 0xbf, 0xea, 0x17, 0x0b,
  0x04, 0x60, 0x32, 0x00, 0xae, 0xf7, 0xe0, 0x74, 0x53, 0x13, 0xe0, 0x1f,
  0x04, 0x66, 0xd8, 0x02, 0x57, 0xa3, 0xc7, 0x4e, 0x3b, 0xfd, 0x3f, 0x57,
  0x3c, 0xcc, 0xf9, 0xab, 0xaf, 0xfd, 0x15, 0x7f, 0x39, 0x54, 0x08, 0x9d,
  0xc3, 0x17, 0xbb, 0xf6, 0x1e, 0x47, 0x52, 0x7c, 0xb8, 0xc7, 0xf3, 0xd1,
  0xe3, 0x8b, 0x53, 0x53, 0xc1, 0x76, 0x09, 0xd0, 0x77, 0x6a, 0x5b, 0x45,
  0x02, 0xae, 0x9d, 0xfe, 0x86, 0xaf, 0x78, 0xeb, 0x74, 0x51, 0x6b, 0xbb,
  0x6e, 0x85, 0x44, 0x2c, 0xfa, 0x74, 0xcd, 0x8a, 0x19, 0x01, 0xae, 0xac,
  0x9a, 0xe8, 0x3b, 0x86, 0x1d, 0xe0, 0x86, 0xd1, 0x31, 0x7b, 0x28, 0xb8,
  0x0e, 0xa0, 0xa3, 0x15, 0xf6, 0xbb, 0x77, 0x61, 0x6d, 0x3c, 0x87, 0xde,
  0xcb, 0x97, 0x60, 0x6c, 0x6e, 0x73, 0xda, 0xaf, 0x96, 0xd6, 0xa2, 0xb3,
  0x4b, 0x8b, 0xf0, 0xc8, 0x70, 0xcc, 0x5d, 0xbb, 0x1b, 0xf1, 0x89, 0x49,
  0x28, 0xaf, 0xba, 0x09, 0x09, 0x8f, 0x29, 0x15, 0x1a, 0xa4, 0x4b, 0x5c,
  0x02, 0xd2, 0xf6, 0xc9, 0x06, 0x58, 0x4c, 0x86, 0x99, 0xaf, 0x35, 0x3b,
  0xe5, 0xd3, 0x22, 0xae, 0xd5, 0xb4, 0x14, 0x8b, 0x03, 0x7d, 0xfe, 0xe3,
  0xc7, 0xd8, 0x0f, 0x98, 0x2b, 0x07, 0x73, 0xfd, 0x30, 0x5c, 0x11, 0x49,
  0x04, 0x7b, 0x9c, 0x14, 0xe2, 0x89, 0x41, 0xe0, 0x49, 0xc5, 0x40, 0xa7,
  0x1a, 0xd4, 0x83, 0x32, 0x57, 0x5d, 0x55, 0x8d, 0x3f, 0x6e, 0x99, 0x8f,
  0x23, 0x87, 0x8f, 0x20, 0xd0, 0x4f, 0x0a, 0x77, 0x1e, 0x07, 0xb0, 0x53,
  0xd3, 0x5d, 0x67, 0xe0, 0xfa, 0xc1, 0x4c, 0xce, 0x85, 0x8a, 0xab, 0x5c,
  0xbb, 0x8e, 0xaf, 0xa5, 0xc7, 0x27, 0x76, 0xa5, 0xbf, 0xc0, 0x70, 0x43,
  0x4a, 0x6f, 0x7b, 0x0f, 0xc4, 0x13, 0x02, 0x1f, 0x0a, 0x69, 0x2f, 0x60,
  0x90, 0x61, 0xe0, 0x9e, 0x1a, 0x16, 0x93, 0x15, 0x14, 0x93, 0x0d, 0x62,
  0xb7, 0x82, 0xc3, 0x65, 0xc3, 0x6d, 0xb4, 0x17, 0x60, 0xb5, 0xe3, 0x6c,
  0xde, 0xd7, 0x58, 0xbc, 0x72, 0x36, 0xe0, 0x27, 0x44, 0x9c, 0x50, 0x80,
  0x80, 0x13, 0x32, 0xdc, 0xef, 0xd2, 0x40, 0x5e, 0x29, 0x47, 0xc0, 0x28,
  0x09, 0x24, 0x62, 0x3e, 0x4c, 0x26, 0x0b, 0x6c, 0x4c, 0x72, 0xd9, 0x15,
  0x01, 0x8b, 0x5d, 0x13, 0xa4, 0xeb, 0xb3, 0x30, 0xbe, 0x68, 0x6a, 0x32,
  0xd1, 0xce, 0x83, 0x02, 0x45, 0x27, 0x94, 0x2a, 0xe3, 0xda, 0x63, 0xa7,
  0x4a, 0xdb, 0x40, 0x46, 0x1c, 0xe7, 0x4e, 0x35, 0xdc, 0x84, 0x1c, 0x78,
  0x84, 0xf8, 0x43, 0x10, 0x2c, 0x75, 0xfe, 0xd2, 0x63, 0xda, 0xee, 0x5c,
  0xab, 0xbf, 0x1f, 0x47, 0xb3, 0xcf, 0x0c, 0xce, 0xe5, 0x73, 0xb1, 0x7f,
  0xff, 0xcb, 0x78, 0xfd, 0x37, 0xe9, 0xce, 0x46, 0xf7, 0x3d, 0x25, 0x42,
  0xa8, 0xfb, 0x6d, 0x4a, 0x23, 0x5f, 0x79, 0xda, 0x25, 0xa0, 0x5d, 0xa9,
  0x5e, 0xac, 0xd4, 0xf6, 0xd7, 0xe5, 0x6e, 0x4f, 0x9b, 0xcd, 0xe1, 0xe1,
  0x60, 0xb1, 0xac, 0x2e, 0x36, 0xf3, 0x4f, 0x45, 0xc7, 0x4a, 0xcb, 0x6f,
  0xa5, 0xe6, 0xe7, 0x9c, 0xd5, 0xd1, 0xef, 0xb4, 0x0b, 0x7e, 0x52, 0x40,
  0x24, 0x1c, 0x2e, 0x8a, 0x1e, 0xd3, 0x76, 0x36, 0x1b, 0x1c, 0x16, 0x0b,
  0xde, 0x02, 0x36, 0x5a, 0x2a, 0x6e, 0x3e, 0x10, 0xc1, 0xc1, 0x94, 0x79,
  0x71, 0xce, 0x06, 0x77, 0x36, 0x0a, 0x8b, 0xe5, 0x7d, 0x35, 0xd5, 0x77,
  0x52, 0x0a, 0x0b, 0x61, 0x75, 0xa5, 0xa0, 0x4b, 0x69, 0x58, 0xe8, 0xce,
  0x71, 0xff, 0xbc, 0xcf, 0xa8, 0xde, 0x52, 0xd2, 0x55, 0x16, 0x91, 0x75,
  0x78, 0xb0, 0x0e, 0xe4, 0x95, 0x74, 0xdc, 0xb4, 0xe2, 0x4a, 0x4c, 0x8b,
  0xa2, 0xe7, 0xf8, 0x2f, 0x37, 0xbc, 0x10, 0xe3, 0x19, 0x32, 0x06, 0xdf,
  0x05, 0x16, 0x87, 0x02, 0x8b, 0xc9, 0x42, 0x71, 0xe9, 0x0d, 0x8c, 0x8f,
  0x9b, 0x38, 0xbc, 0x0e, 0x1c, 0x29, 0xba, 0x7e, 0x43, 0xae, 0xc8, 0xc8,
  0xaf, 0x50, 0xde, 0x71, 0xcd, 0xdf, 0xb7, 0x29, 0x32, 0x54, 0x67, 0x32,
  0x8f, 0x0b, 0x0c, 0xf0, 0xb0, 0x6c, 0x7e, 0xaf, 0x6c, 0xd9, 0xc8, 0x05,
  0x0b, 0x4a, 0x94, 0x2d, 0x05, 0x25, 0xca, 0x29, 0x55, 0x75, 0x2d, 0xe9,
  0xd3, 0xa6, 0x4c, 0xd8, 0x91, 0x38, 0x2d, 0xfc, 0xf9, 0xb8, 0xe8, 0x10,
  0xc0, 0xe7, 0xa1, 0xd7, 0x6d, 0x08, 0x56, 0x0b, 0xdc, 0x39, 0x6e, 0x00,
  0x71, 0x40, 0xa3, 0xd5, 0x03, 0x3d, 0xbd, 0xb8, 0x5a, 0x79, 0x0b, 0xb2,
  0xf2, 0x06, 0xf9, 0x55, 0x79, 0xf3, 0x3b, 0xc7, 0xcb, 0x7a, 0xe8, 0xb0,
  0x0f, 0x4b, 0x2a, 0xf5, 0xfe, 0xaf, 0x63, 0xbf, 0x69, 0x6e, 0xee, 0xcd,
  0x3e, 0x70, 0x41, 0xf1, 0x54, 0xdf, 0x02, 0xfa, 0xae, 0x47, 0xc1, 0xb2,
  0x30, 0x68, 0xb4, 0xdf, 0xec, 0x31, 0x81, 0x92, 0x28, 0x2f, 0x2f, 0xa1,
  0x3f, 0xd7, 0xdd, 0x8d, 0x47, 0x01, 0xee, 0x16, 0xb3, 0x71, 0x60, 0x94,
  0x88, 0xe3, 0x66, 0x34, 0xdb, 0x70, 0xaa, 0xa8, 0x56, 0xa5, 0xd1, 0x1b,
  0xb7, 0xb2, 0x19, 0x9c, 0x2f, 0xf3, 0x64, 0xf7, 0xba, 0x9e, 0xb8, 0x60,
  0x66, 0x9a, 0xdf, 0x88, 0x84, 0x3e, 0x3b, 0xfe, 0xbe, 0x2d, 0x69, 0xff,
  0x57, 0xef, 0x2f, 0x22, 0xbb, 0x5e, 0x8a, 0x76, 0xbc, 0x98, 0x20, 0x9d,
  0xfb, 0x34, 0x1c, 0xea, 0xbf, 0xe5, 0x3c, 0x33, 0x06, 0x9c, 0x99, 0xa9,
  0xc9, 0x5d, 0x0d, 0xcd, 0x5d, 0x1e, 0xcd, 0xf7, 0x35, 0x99, 0x05, 0x65,
  0x3d, 0x39, 0xff, 0xf3, 0x0b, 0xc9, 0x48, 0x9c, 0xb9, 0xd4, 0x98, 0x65,
  0x1f, 0xb0, 0xe6, 0x9c, 0xac, 0xd0, 0xd0, 0x75, 0xf9, 0x27, 0xe0, 0x69,
  0xf0, 0x6f, 0x92, 0x3d, 0xa2, 0xc8, 0x1f, 0x49, 0x3e, 0x67, 0x00, 0x00,
  0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
};
static const unsigned int ICON_HOVER_size = 2014;

// ---------------------------------------------------------------------------
// MumbleLink structs (packed, mirrors GW2 memory layout)
// ---------------------------------------------------------------------------
#pragma pack(push, 1)
struct GW2Context {
    uint8_t  serverAddress[28];
    uint32_t mapId;
    uint32_t mapType;
};
struct MumbleLinkedMem {
    uint32_t uiVersion;
    uint32_t uiTick;
    float    fAvatarPosition[3];
    float    fAvatarFront[3];
    wchar_t  name[256];
    float    fCameraPosition[3];
    float    fCameraFront[3];
    wchar_t  identity[256];
    uint32_t context_len;
    uint8_t  context[256];
};
#pragma pack(pop)

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
void AddonLoad(AddonAPI_t* aApi);
void AddonUnload();
void AddonRender();
void AddonOptions();

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------
static AddonAPI_t*       APIDefs  = nullptr;
static AddonDefinition_t AddonDef{};
static bool g_WindowVisible  = false;
static bool   g_OverlayVisible  = false;
static bool   g_OverlayLocked   = true;
static NexusLinkData_t* g_NexusLink = nullptr;
static ImVec2 g_OverlayPos     = {-1.f, -1.f};  // sentinel: computed on first render
static bool g_MapWindowVisible = false;
static bool g_ShowQAIcon     = true;

static MapPanel g_MapPanel;

static int  g_SelectedFish    = -1;
static int  g_LastDetailFish  =  0;  // persists last viewed fish; never goes back to -1
static char g_SearchBuf[128]  = {};
static int  g_FilterBait      = 0;
static int  g_FilterTime      = 0;
static bool g_ShowCurrentOnly = false;
static bool g_MissingOnly     = false;
static std::string g_FilterMap;
static std::string g_CurrentMapName;

static std::vector<std::string> g_Favourites;
static std::mutex               g_FavMutex;

static bool g_SwitchToDatabase = false;

static std::vector<int> g_SortedFishIndices;

enum class FishSortMode { Default, Alphabetical, Rarity, MapName };
static FishSortMode g_SortMode = FishSortMode::Default;
static bool         g_SortAsc  = true;
static bool         g_SortDirty = true;

static const int   HOLE_RESPAWN_SECONDS = 600;
static const int   HOLE_DWELL_SECONDS   = 30;
static const float HOLE_PROXIMITY_M     = 50.f;

struct HoleDwellState {
    std::chrono::steady_clock::time_point enteredAt;
    bool inside = false;
};
static std::unordered_map<int, HoleDwellState>                        g_HoleDwell;
static std::unordered_map<int, std::chrono::steady_clock::time_point> g_HoleRespawn;
static std::mutex g_RespawnMu;

static int g_NotifyLeadSeconds = 180;
struct Toast {
    std::string message;
    std::string subtext;
    float       expiresAt;
};
static std::vector<Toast> g_Toasts;
static std::mutex         g_ToastMu;
static std::unordered_map<int, uint32_t> g_LastNotifiedCycle;

struct FavFishEntry {
    int         fishIdx;   // index into FISH_TABLE
    uint32_t    mapId;     // mapId for "Open Map" button (0 if not found)
};

struct FavNotification {
    std::vector<FavFishEntry> fish;
    float    createdAt  = 0.f;  // ImGui::GetTime() when fired
    bool     dismissed  = false;
};

static FavNotification                  g_FavNotif;
static bool                             g_FavNotifActive  = false;
static bool                             g_FavNotifEnabled = true;
static bool                             g_FavNotifLocked  = true;
static ImVec2                           g_FavNotifPos     = {-1.f, -1.f};  // sentinel: computed on first render
static int                              g_AutoDismissSeconds = 0;
// Tracks which Tyrian cycle (time(0)/7200) each fish last fired, to prevent
// re-firing within the same 7200-second cycle.
static std::unordered_map<int, int64_t> g_LastNotifiedCycleWall;

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------
static std::string SettingsPath() {
    return std::string(APIDefs->Paths_GetAddonDirectory("CastAway")) + "/settings.json";
}

static void SaveSettings() {
    try {
        nlohmann::json j;
        {
            std::lock_guard<std::mutex> lk(g_FavMutex);
            j["favourites"] = g_Favourites;
        }
        j["overlay_visible"]     = g_OverlayVisible;
        j["overlay_locked"]      = g_OverlayLocked;
        j["overlay_pos_x"]       = g_OverlayPos.x;
        j["overlay_pos_y"]       = g_OverlayPos.y;
        j["show_qa_icon"]        = g_ShowQAIcon;
        j["notify_lead_seconds"]   = g_NotifyLeadSeconds;
        j["auto_dismiss_seconds"]  = g_AutoDismissSeconds;
        j["fav_notif_enabled"]     = g_FavNotifEnabled;
        j["fav_notif_locked"]      = g_FavNotifLocked;
        j["fav_notif_pos_x"]       = g_FavNotifPos.x;
        j["fav_notif_pos_y"]       = g_FavNotifPos.y;

        std::string path = SettingsPath();
        std::filesystem::create_directories(
            std::filesystem::path(path).parent_path());
        std::ofstream f(path);
        if (f.is_open()) f << j.dump(2);
    } catch (...) {}
}

static void LoadSettings() {
    try {
        std::ifstream f(SettingsPath());
        if (!f.is_open()) return;
        nlohmann::json j = nlohmann::json::parse(f, nullptr, false);
        if (j.is_discarded()) return;

        if (j.contains("favourites") && j["favourites"].is_array()) {
            std::lock_guard<std::mutex> lk(g_FavMutex);
            g_Favourites.clear();
            for (auto& v : j["favourites"])
                if (v.is_string()) g_Favourites.push_back(v.get<std::string>());
        }
        if (j.contains("overlay_visible"))     g_OverlayVisible    = j["overlay_visible"].get<bool>();
        if (j.contains("overlay_locked"))      g_OverlayLocked     = j["overlay_locked"].get<bool>();
        if (j.contains("overlay_pos_x") && j.contains("overlay_pos_y")) {
            g_OverlayPos.x = j["overlay_pos_x"].get<float>();
            g_OverlayPos.y = j["overlay_pos_y"].get<float>();
        }
        if (j.contains("show_qa_icon"))        g_ShowQAIcon        = j["show_qa_icon"].get<bool>();
        if (j.contains("notify_lead_seconds"))  g_NotifyLeadSeconds  = j["notify_lead_seconds"].get<int>();
        if (j.contains("auto_dismiss_seconds")) g_AutoDismissSeconds = j["auto_dismiss_seconds"].get<int>();
        if (j.contains("fav_notif_enabled"))    g_FavNotifEnabled    = j["fav_notif_enabled"].get<bool>();
        if (j.contains("fav_notif_locked"))     g_FavNotifLocked     = j["fav_notif_locked"].get<bool>();
        if (j.contains("fav_notif_pos_x") && j.contains("fav_notif_pos_y")) {
            g_FavNotifPos.x = j["fav_notif_pos_x"].get<float>();
            g_FavNotifPos.y = j["fav_notif_pos_y"].get<float>();
        }
    } catch (...) {}
}

// ---------------------------------------------------------------------------
// Filter helpers
// ---------------------------------------------------------------------------
static bool FishMatchesFilter(int fishIdx) {
    const Fish& f = FISH_TABLE[fishIdx];

    // Name search (case-insensitive)
    if (g_SearchBuf[0]) {
        std::string haystack = f.name;
        std::string needle   = g_SearchBuf;
        std::transform(haystack.begin(), haystack.end(), haystack.begin(), ::tolower);
        std::transform(needle.begin(),   needle.end(),   needle.begin(),   ::tolower);
        if (haystack.find(needle) == std::string::npos) return false;
    }

    // Bait filter
    if (g_FilterBait > 0 && (int)f.bait != g_FilterBait) return false;

    // Time filter
    if (g_ShowCurrentOnly) {
        TimeOfDay cur = GetCurrentTimeOfDay();
        if (f.time != TimeOfDay::Any && f.time != cur) return false;
    } else if (g_FilterTime > 0 && f.time != TimeOfDay::Any && (int)f.time != g_FilterTime) {
        return false;
    }

    // Map filter
    if (!g_FilterMap.empty() && g_FilterMap != std::string(f.map ? f.map : "")) return false;

    // Missing only — hide fish already caught
    if (g_MissingOnly && g_AchTracker.hoarded && g_AchTracker.IsCaught(fishIdx)) return false;

    return true;
}

static bool IsFavourite(const char* name) {
    std::lock_guard<std::mutex> lk(g_FavMutex);
    return std::find(g_Favourites.begin(), g_Favourites.end(), name) != g_Favourites.end();
}

static void ToggleFavourite(const char* name) {
    std::lock_guard<std::mutex> lk(g_FavMutex);
    auto it = std::find(g_Favourites.begin(), g_Favourites.end(), name);
    if (it != g_Favourites.end()) g_Favourites.erase(it);
    else                          g_Favourites.push_back(name);
}

// ---------------------------------------------------------------------------
// Day/Night timeline bar
// ---------------------------------------------------------------------------
static void RenderDayNightBar(float windowWidth) {
    static const float BAR_H   = 38.f;
    static const float TICK_H  = 18.f;
    static const float TOTAL_H = BAR_H + TICK_H;

    ImDrawList* dl     = ImGui::GetWindowDrawList();
    ImVec2      origin = ImGui::GetCursorScreenPos();

    // Gradient colour stops: {tyrianHour, R, G, B}. Matches GW2 phase boundaries:
    //   00:00–05:00 Night, 05:00–06:00 Dawn, 06:00–20:00 Day, 20:00–21:00 Dusk, 21:00–24:00 Night
    struct Stop { float h; uint8_t r, g, b; };
    static const Stop stops[] = {
        { 0.00f, 15,  15,  50  },   // deep night
        { 5.00f, 25,  20,  70  },   // night, approaching dawn
        { 5.50f, 196, 107, 26  },   // mid-dawn (orange)
        { 6.00f, 110, 180, 230 },   // day begins
        {12.00f, 180, 220, 255 },   // noon
        {18.00f, 130, 200, 230 },   // late afternoon
        {20.00f, 110, 180, 230 },   // day end
        {20.50f, 212, 112, 42  },   // mid-dusk (orange)
        {21.00f, 30,  20,  70  },   // night begins
        {24.00f, 15,  15,  50  },   // deep night
    };
    static const int NSTOPS = (int)(sizeof(stops) / sizeof(stops[0]));

    // Current time / phase
    float tyrHour = GetTyrianHour();
    TimeOfDay phase = GetCurrentTimeOfDay();
    uint32_t secLeft = SecondsUntilNextSlot();

    // Scrolled view: bar shows [now - 12h, now + 12h], with "now" pinned to the centre.
    // Each pixel column is sampled from the gradient at its corresponding Tyrian hour.
    const float leftH = tyrHour - 12.f;
    auto colorAt = [&](float h) -> ImU32 {
        h = fmodf(h + 24.f * 100.f, 24.f); // normalise into [0,24)
        for (int i = 0; i < NSTOPS - 1; i++) {
            if (h >= stops[i].h && h < stops[i+1].h) {
                float t = (h - stops[i].h) / (stops[i+1].h - stops[i].h);
                uint8_t r = (uint8_t)(stops[i].r + (stops[i+1].r - stops[i].r) * t);
                uint8_t g = (uint8_t)(stops[i].g + (stops[i+1].g - stops[i].g) * t);
                uint8_t b = (uint8_t)(stops[i].b + (stops[i+1].b - stops[i].b) * t);
                return IM_COL32(r, g, b, 255);
            }
        }
        return IM_COL32(stops[NSTOPS-1].r, stops[NSTOPS-1].g, stops[NSTOPS-1].b, 255);
    };
    const int STRIPS = 96;
    for (int s = 0; s < STRIPS; s++) {
        float t0 = (float)s       / (float)STRIPS;
        float t1 = (float)(s + 1) / (float)STRIPS;
        ImU32 c0 = colorAt(leftH + t0 * 24.f);
        ImU32 c1 = colorAt(leftH + t1 * 24.f);
        float x0 = origin.x + t0 * windowWidth;
        float x1 = origin.x + t1 * windowWidth;
        dl->AddRectFilledMultiColor(
            {x0, origin.y}, {x1, origin.y + BAR_H},
            c0, c1, c1, c0);
    }

    // Centre "now" line
    float centerX = origin.x + windowWidth * 0.5f;
    dl->AddLine({centerX, origin.y}, {centerX, origin.y + BAR_H},
                IM_COL32(255, 255, 255, 230), 2.f);
    dl->AddLine({centerX + 1.f, origin.y}, {centerX + 1.f, origin.y + BAR_H},
                IM_COL32(0, 0, 0, 140), 1.f);

    // Left label: "HH:MM  PhaseName"
    char timeLabel[64];
    {
        float th = GetTyrianHour();      // 0..24 Tyrian clock
        uint32_t hh = (uint32_t)th;
        uint32_t mm = (uint32_t)((th - (float)hh) * 60.0f);
        snprintf(timeLabel, sizeof(timeLabel), "%02u:%02u  %s", hh, mm, TimeOfDayName(phase));
    }
    dl->AddText({origin.x + 4.f, origin.y + 4.f},
                IM_COL32(255, 255, 255, 230), timeLabel);

    // Right label: "→ NextPhase in Xm XXs"
    char untilLabel[48];
    snprintf(untilLabel, sizeof(untilLabel), "%s in %um %02us",
             TimeOfDayName(GetNextPhase()), secLeft / 60, secLeft % 60);
    ImVec2 tsz = ImGui::CalcTextSize(untilLabel);
    dl->AddText({origin.x + windowWidth - tsz.x - 4.f, origin.y + 4.f},
                IM_COL32(255, 255, 255, 200), untilLabel);

    // Tick strip background
    dl->AddRectFilled(
        {origin.x, origin.y + BAR_H},
        {origin.x + windowWidth, origin.y + TOTAL_H},
        IM_COL32(15, 15, 20, 220));

    // Tick marks at every 3 Tyrian hours, scrolled with the gradient. The bar shows
    // a 24-hour window so each hour appears exactly once.
    static const float tickHours[]  = { 0.f,  3.f,  6.f,  9.f, 12.f, 15.f, 18.f, 21.f };
    static const char* tickLabels[] = { "0",  "3",  "6",  "9", "12", "15", "18", "21" };
    static const int   N_TICKS      = (int)(sizeof(tickHours) / sizeof(tickHours[0]));
    for (int i = 0; i < N_TICKS; i++) {
        float d = tickHours[i] - leftH;
        d = fmodf(d + 24.f * 100.f, 24.f);
        float tx = origin.x + (d / 24.f) * windowWidth;
        // Suppress ticks within a few pixels of the centre "now" line to avoid overlap
        if (fabsf(tx - centerX) < 12.f) continue;
        dl->AddLine({tx, origin.y + BAR_H},
                    {tx, origin.y + TOTAL_H - 4.f},
                    IM_COL32(180, 180, 180, 160), 1.f);
        ImVec2 lsz = ImGui::CalcTextSize(tickLabels[i]);
        float lx = tx - lsz.x * 0.5f;
        if (lx < origin.x) lx = origin.x;
        if (lx + lsz.x > origin.x + windowWidth)
            lx = origin.x + windowWidth - lsz.x;
        dl->AddText({lx, origin.y + BAR_H + 2.f},
                    IM_COL32(200, 200, 200, 200), tickLabels[i]);
    }

    // "Now" tick — show the current Tyrian hour right under the centre line
    {
        char nowTick[8];
        snprintf(nowTick, sizeof(nowTick), "%02u", (uint32_t)tyrHour);
        ImVec2 lsz = ImGui::CalcTextSize(nowTick);
        dl->AddLine({centerX, origin.y + BAR_H},
                    {centerX, origin.y + TOTAL_H - 4.f},
                    IM_COL32(255, 255, 255, 220), 1.5f);
        dl->AddText({centerX - lsz.x * 0.5f, origin.y + BAR_H + 2.f},
                    IM_COL32(255, 255, 255, 235), nowTick);
    }

    // Advance ImGui cursor past the bar
    ImGui::Dummy({windowWidth, TOTAL_H});
}

// ---------------------------------------------------------------------------
// Conditions overlay — mini day/night bar
// ---------------------------------------------------------------------------
static void RenderOverlay() {
    if (!g_OverlayVisible || !(g_NexusLink && g_NexusLink->IsGameplay)) return;

    ImGuiIO& io = ImGui::GetIO();

    bool posWasReset = (g_OverlayPos.x < 0.f);
    if (posWasReset)
        g_OverlayPos = {io.DisplaySize.x / 3.f, io.DisplaySize.y / 3.f};

    static bool s_wasLocked = true;
    bool justUnlocked = (s_wasLocked && !g_OverlayLocked);
    s_wasLocked = g_OverlayLocked;

    if (g_OverlayLocked || justUnlocked || posWasReset)
        ImGui::SetNextWindowPos(g_OverlayPos, ImGuiCond_Always);

    static const float W      = 240.f;
    static const float BAR_H  = 22.f;
    static const float TICK_H = 13.f;
    static const float TOTAL_H = BAR_H + TICK_H;

    ImGui::SetNextWindowSize({W, TOTAL_H}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.f);  // bar fills the whole window

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration    |
        ImGuiWindowFlags_NoNav           |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (g_OverlayLocked)
        flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::Begin("##CastAwayOverlay", nullptr, flags)) {
        ImDrawList* dl     = ImGui::GetWindowDrawList();
        ImVec2      origin = ImGui::GetCursorScreenPos();

        struct Stop { float h; uint8_t r, g, b; };
        static const Stop stops[] = {
            { 0.00f, 15,  15,  50  },
            { 5.00f, 25,  20,  70  },
            { 5.50f, 196, 107, 26  },
            { 6.00f, 110, 180, 230 },
            {12.00f, 180, 220, 255 },
            {18.00f, 130, 200, 230 },
            {20.00f, 110, 180, 230 },
            {20.50f, 212, 112, 42  },
            {21.00f, 30,  20,  70  },
            {24.00f, 15,  15,  50  },
        };
        static const int NSTOPS = (int)(sizeof(stops) / sizeof(stops[0]));

        float tyrHour = GetTyrianHour();
        float leftH   = tyrHour - 12.f;

        auto colorAt = [&](float h) -> ImU32 {
            h = fmodf(h + 24.f * 100.f, 24.f);
            for (int i = 0; i < NSTOPS - 1; i++) {
                if (h >= stops[i].h && h < stops[i+1].h) {
                    float t = (h - stops[i].h) / (stops[i+1].h - stops[i].h);
                    uint8_t r = (uint8_t)(stops[i].r + (stops[i+1].r - stops[i].r) * t);
                    uint8_t g = (uint8_t)(stops[i].g + (stops[i+1].g - stops[i].g) * t);
                    uint8_t b = (uint8_t)(stops[i].b + (stops[i+1].b - stops[i].b) * t);
                    return IM_COL32(r, g, b, 255);
                }
            }
            return IM_COL32(stops[NSTOPS-1].r, stops[NSTOPS-1].g, stops[NSTOPS-1].b, 255);
        };

        // Gradient bar
        const int STRIPS = 64;
        for (int s = 0; s < STRIPS; s++) {
            float t0 = (float)s       / (float)STRIPS;
            float t1 = (float)(s + 1) / (float)STRIPS;
            ImU32 c0 = colorAt(leftH + t0 * 24.f);
            ImU32 c1 = colorAt(leftH + t1 * 24.f);
            dl->AddRectFilledMultiColor(
                {origin.x + t0 * W, origin.y},
                {origin.x + t1 * W, origin.y + BAR_H},
                c0, c1, c1, c0);
        }

        // Centre "now" line
        float cx = origin.x + W * 0.5f;
        dl->AddLine({cx, origin.y}, {cx, origin.y + BAR_H},
                    IM_COL32(255, 255, 255, 230), 2.f);
        dl->AddLine({cx + 1.f, origin.y}, {cx + 1.f, origin.y + BAR_H},
                    IM_COL32(0, 0, 0, 120), 1.f);

        // Phase label (left) and countdown (right) overlaid on bar
        TimeOfDay phase   = GetCurrentTimeOfDay();
        uint32_t  secLeft = SecondsUntilNextSlot();
        char leftLbl[32];
        {
            uint32_t hh = (uint32_t)tyrHour;
            uint32_t mm = (uint32_t)((tyrHour - (float)hh) * 60.f);
            snprintf(leftLbl, sizeof(leftLbl), "%02u:%02u %s", hh, mm, TimeOfDayName(phase));
        }
        char rightLbl[32];
        snprintf(rightLbl, sizeof(rightLbl), "%s %um%02us",
                 TimeOfDayName(GetNextPhase()), secLeft / 60, secLeft % 60);

        dl->AddText({origin.x + 3.f, origin.y + 3.f},
                    IM_COL32(255, 255, 255, 220), leftLbl);
        ImVec2 rsz = ImGui::CalcTextSize(rightLbl);
        dl->AddText({origin.x + W - rsz.x - 3.f, origin.y + 3.f},
                    IM_COL32(255, 255, 255, 200), rightLbl);

        // Tick strip
        dl->AddRectFilled(
            {origin.x, origin.y + BAR_H},
            {origin.x + W, origin.y + TOTAL_H},
            IM_COL32(15, 15, 20, 220));

        static const float tickHours[]  = { 0.f, 6.f, 12.f, 18.f };
        static const char* tickLabels[] = { "0", "6", "12", "18" };
        for (int i = 0; i < 4; i++) {
            float d  = fmodf(tickHours[i] - leftH + 24.f * 100.f, 24.f);
            float tx = origin.x + (d / 24.f) * W;
            if (fabsf(tx - cx) < 10.f) continue;
            dl->AddLine({tx, origin.y + BAR_H}, {tx, origin.y + TOTAL_H - 3.f},
                        IM_COL32(180, 180, 180, 150), 1.f);
            ImVec2 lsz = ImGui::CalcTextSize(tickLabels[i]);
            dl->AddText({tx - lsz.x * 0.5f, origin.y + BAR_H + 1.f},
                        IM_COL32(200, 200, 200, 190), tickLabels[i]);
        }
        // "Now" tick
        {
            char nowTick[8];
            snprintf(nowTick, sizeof(nowTick), "%02u", (uint32_t)tyrHour);
            ImVec2 lsz = ImGui::CalcTextSize(nowTick);
            dl->AddLine({cx, origin.y + BAR_H}, {cx, origin.y + TOTAL_H - 3.f},
                        IM_COL32(255, 255, 255, 210), 1.5f);
            dl->AddText({cx - lsz.x * 0.5f, origin.y + BAR_H + 1.f},
                        IM_COL32(255, 255, 255, 230), nowTick);
        }

        ImGui::Dummy({W, TOTAL_H});

        if (!g_OverlayLocked) {
            ImVec2 p = ImGui::GetWindowPos();
            if (p.x != g_OverlayPos.x || p.y != g_OverlayPos.y)
                g_OverlayPos = p;
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

// ---------------------------------------------------------------------------
// Toast helpers
// ---------------------------------------------------------------------------
static void PushToast(const std::string& message, const std::string& subtext,
                      float durationSec = 6.f) {
    float now = (float)ImGui::GetTime();
    std::lock_guard<std::mutex> lk(g_ToastMu);
    for (auto& t : g_Toasts)
        if (t.message == message && t.expiresAt > now) return;
    g_Toasts.push_back({message, subtext, now + durationSec});
}

// ---------------------------------------------------------------------------
// Toast render
// ---------------------------------------------------------------------------
static void RenderToasts() {
    float now = (float)ImGui::GetTime();
    std::lock_guard<std::mutex> lk(g_ToastMu);

    g_Toasts.erase(
        std::remove_if(g_Toasts.begin(), g_Toasts.end(),
                       [now](const Toast& t){ return t.expiresAt <= now; }),
        g_Toasts.end());

    if (g_Toasts.empty()) return;

    ImGuiIO& io = ImGui::GetIO();
    float yBase = io.DisplaySize.y - 20.f;

    for (int i = (int)g_Toasts.size() - 1; i >= 0; i--) {
        const Toast& t = g_Toasts[i];
        float remaining = t.expiresAt - now;
        float alpha = (remaining < 1.f) ? remaining : 1.f;

        ImVec2 sz{280.f, t.subtext.empty() ? 36.f : 52.f};
        yBase -= sz.y + 4.f;

        ImGui::SetNextWindowPos({12.f, yBase}, ImGuiCond_Always);
        ImGui::SetNextWindowSize(sz, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.80f * alpha);

        char wid[32];
        snprintf(wid, sizeof(wid), "##toast%d", i);
        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration    |
            ImGuiWindowFlags_NoInputs        |
            ImGuiWindowFlags_NoNav           |
            ImGuiWindowFlags_NoMove          |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
        if (ImGui::Begin(wid, nullptr, flags)) {
            ImGui::TextUnformatted(t.message.c_str());
            if (!t.subtext.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(200, 200, 200, 220));
                ImGui::TextUnformatted(t.subtext.c_str());
                ImGui::PopStyleColor();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
}

// ---------------------------------------------------------------------------
// Hole dwell tracking
// ---------------------------------------------------------------------------
static void UpdateHoleDwellTimers(int mapId, float gx, float gz) {
    auto now = std::chrono::steady_clock::now();
    for (int i = 0; i < HOLE_COUNT; i++) {
        const FishingHole& h = HOLE_TABLE[i];
        if ((int)h.mapId != mapId) continue;
        if (h.game_x == 0.f && h.game_z == 0.f) continue;

        float dx   = gx - h.game_x;
        float dz   = gz - h.game_z;
        float dist = sqrtf(dx*dx + dz*dz);
        bool  inside = (dist < HOLE_PROXIMITY_M);

        HoleDwellState& ds = g_HoleDwell[i];
        if (inside && !ds.inside) {
            ds.inside    = true;
            ds.enteredAt = now;
        } else if (!inside && ds.inside) {
            ds.inside = false;
            auto dwellSecs = std::chrono::duration_cast<std::chrono::seconds>(
                now - ds.enteredAt).count();
            if (dwellSecs >= HOLE_DWELL_SECONDS) {
                std::lock_guard<std::mutex> lk(g_RespawnMu);
                g_HoleRespawn[i] = now;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Time window notifications
// ---------------------------------------------------------------------------
static uint32_t MapIdForFishRegion(const char* regionName) {
    if (!regionName) return 0;
    for (int i = 0; i < HOLE_COUNT; i++) {
        const FishingHole& h = HOLE_TABLE[i];
        if (h.map && strcmp(h.map, regionName) == 0)
            return h.mapId;
    }
    return 0;
}

static void CheckTimeWindowNotifications() {
    if (!g_FavNotifEnabled) return;
    static auto lastCheck = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastCheck).count() < 1)
        return;
    lastCheck = now;

    int64_t wallCycle = (int64_t)(time(nullptr) / 7200);

    std::vector<std::string> favsCopy;
    {
        std::lock_guard<std::mutex> lk(g_FavMutex);
        favsCopy = g_Favourites;
    }

    std::vector<FavFishEntry> triggered;
    for (int i = 0; i < FISH_COUNT; i++) {
        const Fish& f = FISH_TABLE[i];
        if (f.time == TimeOfDay::Any) continue;

        bool fav = false;
        for (auto& n : favsCopy) if (n == f.name) { fav = true; break; }
        if (!fav) continue;

        uint32_t secs = SecondsUntilPhase(f.time);
        // secs == 0 means the phase just started; skip (already-in-window rule)
        if (secs == 0 || secs > (uint32_t)g_NotifyLeadSeconds) continue;

        auto it = g_LastNotifiedCycleWall.find(i);
        if (it != g_LastNotifiedCycleWall.end() && it->second == wallCycle) continue;
        g_LastNotifiedCycleWall[i] = wallCycle;

        triggered.push_back({i, MapIdForFishRegion(f.map)});
    }

    if (triggered.empty()) return;

    // Merge into any existing active notification, or create a new one
    if (g_FavNotifActive && !g_FavNotif.dismissed) {
        bool added = false;
        for (auto& e : triggered) {
            bool already = false;
            for (auto& ex : g_FavNotif.fish)
                if (ex.fishIdx == e.fishIdx) { already = true; break; }
            if (!already) { g_FavNotif.fish.push_back(e); added = true; }
        }
        if (added) g_FavNotif.createdAt = (float)ImGui::GetTime();
    } else {
        g_FavNotif          = FavNotification{};
        g_FavNotif.fish     = triggered;
        g_FavNotif.createdAt = (float)ImGui::GetTime();
        g_FavNotif.dismissed = false;
        g_FavNotifActive    = true;
    }
}

// ---------------------------------------------------------------------------
// Nearby hole HUD
// ---------------------------------------------------------------------------
static void CheckNearbyHoles(int mapId, float gx, float gz) {
    if (mapId == 0) return;
    TimeOfDay curTime = GetCurrentTimeOfDay();

    for (int i = 0; i < HOLE_COUNT; i++) {
        const FishingHole& h = HOLE_TABLE[i];
        if ((int)h.mapId != mapId) continue;
        if (h.game_x == 0.f && h.game_z == 0.f) continue;

        float dx   = gx - h.game_x;
        float dz   = gz - h.game_z;
        float dist = sqrtf(dx*dx + dz*dz);
        if (dist >= HOLE_PROXIMITY_M) continue;

        // Collect catchable fish
        std::vector<const Fish*> catchable;
        for (int fi = 0; fi < (int)h.fishCount; fi++) {
            uint16_t fid = h.fishIds[fi];
            if (fid >= (uint16_t)FISH_COUNT) continue;
            const Fish& ff = FISH_TABLE[fid];
            if (ff.time == TimeOfDay::Any || ff.time == curTime)
                catchable.push_back(&ff);
        }

        ImGuiIO& io = ImGui::GetIO();
        float w      = 240.f;
        float lineH  = ImGui::GetTextLineHeightWithSpacing();
        float winH   = 28.f + lineH * (float)(catchable.empty() ? 1 : (int)catchable.size());
        ImGui::SetNextWindowPos(
            {io.DisplaySize.x * 0.5f - w * 0.5f, io.DisplaySize.y * 0.35f},
            ImGuiCond_Always);
        ImGui::SetNextWindowSize({w, winH}, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.75f);

        char wid[32];
        snprintf(wid, sizeof(wid), "##hole%d", i);
        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration    |
            ImGuiWindowFlags_NoInputs        |
            ImGuiWindowFlags_NoNav           |
            ImGuiWindowFlags_NoMove          |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        if (ImGui::Begin(wid, nullptr, flags)) {
            ImGui::TextColored({1.f, 0.85f, 0.3f, 1.f}, "%s", h.name);
            if (catchable.empty()) {
                ImGui::TextDisabled("No fish active now");
            } else {
                for (const Fish* fp : catchable)
                    ImGui::BulletText("%s", fp->name);
            }
        }
        ImGui::End();
        break; // Show only the nearest hole
    }
}

// ---------------------------------------------------------------------------
// Rarity colour helper (GW2 standard palette)
// ---------------------------------------------------------------------------
static ImVec4 RarityColor(const char* rarity) {
    if (!rarity)                         return {0.87f, 0.87f, 0.87f, 1.f};
    if (strcmp(rarity, "Fine")       ==0) return {0.384f, 0.644f, 0.855f, 1.f};
    if (strcmp(rarity, "Masterwork") ==0) return {0.102f, 0.576f, 0.024f, 1.f};
    if (strcmp(rarity, "Rare")       ==0) return {0.988f, 0.816f, 0.043f, 1.f};
    if (strcmp(rarity, "Exotic")     ==0) return {0.788f, 0.494f, 0.063f, 1.f};
    if (strcmp(rarity, "Ascended")   ==0) return {0.984f, 0.243f, 0.553f, 1.f};
    if (strcmp(rarity, "Legendary")  ==0) return {0.659f, 0.341f, 0.898f, 1.f};
    return {0.87f, 0.87f, 0.87f, 1.f};
}

// ---------------------------------------------------------------------------
// ImGui primitive draw helpers
// ---------------------------------------------------------------------------
// Coin price rendering (gold/silver/copper with coloured coin circles)
static const float COIN_RADIUS = 4.0f;
static const float COIN_GAP    = 2.0f;

static void DrawCoinInline(ImU32 fill, ImU32 edge) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float  cy  = pos.y + ImGui::GetTextLineHeight() * 0.5f;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddCircleFilled(ImVec2(pos.x + COIN_RADIUS, cy), COIN_RADIUS, fill, 12);
    dl->AddCircle      (ImVec2(pos.x + COIN_RADIUS, cy), COIN_RADIUS, edge, 12, 1.2f);
    ImGui::Dummy(ImVec2(COIN_RADIUS * 2, ImGui::GetTextLineHeight()));
    ImGui::SameLine(0, COIN_GAP);
}

static void RenderCoinPrice(int copper) {
    if (copper <= 0) { ImGui::TextDisabled("—"); return; }
    int g = copper / 10000;
    int s = (copper % 10000) / 100;
    int c = copper % 100;
    static const ImVec4 goldCol   = {1.00f, 0.84f, 0.00f, 1.f};
    static const ImVec4 silverCol = {0.75f, 0.75f, 0.78f, 1.f};
    static const ImVec4 copperCol = {0.72f, 0.45f, 0.20f, 1.f};
    if (g > 0) {
        ImGui::TextColored(goldCol, "%d", g);
        ImGui::SameLine(0, 1);
        DrawCoinInline(IM_COL32(255,215,0,255), IM_COL32(180,140,0,255));
        if (s == 0 && c == 0) return;
    }
    if (g > 0 || s > 0) {
        if (g > 0 && s < 10) ImGui::TextColored(silverCol, "0%d", s);
        else                  ImGui::TextColored(silverCol, "%d",  s);
        ImGui::SameLine(0, 1);
        DrawCoinInline(IM_COL32(192,192,200,255), IM_COL32(120,120,135,255));
    }
    if ((g > 0 || s > 0) && c < 10) ImGui::TextColored(copperCol, "0%d", c);
    else                              ImGui::TextColored(copperCol, "%d",  c);
    ImGui::SameLine(0, 1);
    DrawCoinInline(IM_COL32(184,115,51,255), IM_COL32(120,70,30,255));
}

static ImU32 ChipTimeColor(TimeOfDay t) {
    switch (t) {
        case TimeOfDay::Dawn:  return IM_COL32(230,140, 60,255);
        case TimeOfDay::Day:   return IM_COL32(230,210, 60,255);
        case TimeOfDay::Dusk:  return IM_COL32(210,100, 40,255);
        case TimeOfDay::Night: return IM_COL32(100,140,210,255);
        default:               return IM_COL32(153,153,153,255);
    }
}

static ImU32 ChipWaterColor(WaterType w) {
    switch (w) {
        case WaterType::Freshwater: return IM_COL32( 80,140,220,255);
        case WaterType::Saltwater:  return IM_COL32( 60,180,180,255);
        default:                    return IM_COL32(160,100,200,255);
    }
}

// Small primitive icons for attribute chips
static void DrawChipBaitIcon(ImDrawList* dl, ImVec2 p, float s, ImU32 col) {
    // J-hook: vertical shaft + curved hook
    dl->AddLine({p.x+s*0.45f, p.y+s*0.05f}, {p.x+s*0.45f, p.y+s*0.5f}, col, 1.2f);
    dl->AddBezierCubic({p.x+s*0.45f, p.y+s*0.5f}, {p.x+s*0.45f, p.y+s*0.95f},
                       {p.x+s*0.9f,  p.y+s*0.95f}, {p.x+s*0.9f, p.y+s*0.68f}, col, 1.2f, 6);
}

static void DrawChipTimeIcon(ImDrawList* dl, ImVec2 p, float s, TimeOfDay t, ImU32 col) {
    float cx = p.x+s*0.5f, cy = p.y+s*0.5f, r = s*0.38f;
    switch (t) {
        case TimeOfDay::Day:
            dl->AddCircleFilled({cx,cy}, r*0.65f, col, 8);
            for (int i = 0; i < 4; i++) {
                float a = (float)M_PI*0.25f + i*(float)M_PI*0.5f;
                dl->AddLine({cx+cosf(a)*r*0.78f, cy+sinf(a)*r*0.78f},
                            {cx+cosf(a)*r,       cy+sinf(a)*r}, col, 1.f);
            }
            break;
        case TimeOfDay::Night:
            dl->AddCircle({cx, cy}, r, col, 12, 1.2f);
            dl->AddCircleFilled({cx+r*0.45f, cy-r*0.35f}, 1.8f, col);
            break;
        case TimeOfDay::Dawn:
        case TimeOfDay::Dusk: {
            float hy = cy + r*0.15f;
            dl->AddLine({p.x+1.f, hy}, {p.x+s-1.f, hy}, col, 1.f);
            for (int i = 1; i <= 8; i++) {
                float a0 = (float)M_PI+(i-1)*(float)M_PI/8.f, a1 = (float)M_PI+i*(float)M_PI/8.f;
                dl->AddLine({cx+cosf(a0)*r, hy+sinf(a0)*r},
                           {cx+cosf(a1)*r, hy+sinf(a1)*r}, col, 1.2f);
            }
            break;
        }
        default:
            dl->AddCircle({cx, cy}, r, col, 8, 1.2f);
            break;
    }
}

static void DrawChipWaterIcon(ImDrawList* dl, ImVec2 p, float s, WaterType w, ImU32 col) {
    float cx = p.x+s*0.5f;
    switch (w) {
        case WaterType::Freshwater: {
            float cy = p.y+s*0.62f;
            dl->AddCircleFilled({cx, cy}, s*0.27f, col, 8);
            ImVec2 tri[3] = {{cx, p.y+s*0.08f}, {cx-s*0.24f, cy-s*0.05f}, {cx+s*0.24f, cy-s*0.05f}};
            dl->AddTriangleFilled(tri[0], tri[1], tri[2], col);
            break;
        }
        case WaterType::Saltwater:
            for (int i = 0; i < 8; i++) {
                float x0 = p.x+s*i/8.f, x1 = p.x+s*(i+1)/8.f;
                float a0 = i*(float)M_PI*0.5f, a1 = (i+1)*(float)M_PI*0.5f;
                dl->AddLine({x0, p.y+s*0.5f+sinf(a0)*s*0.22f},
                           {x1, p.y+s*0.5f+sinf(a1)*s*0.22f}, col, 1.2f);
            }
            break;
        default: {
            float r = s*0.36f, cy = p.y+s*0.5f;
            ImVec2 di[4] = {{cx,cy-r},{cx+r*0.6f,cy},{cx,cy+r},{cx-r*0.6f,cy}};
            dl->AddPolyline(di, 4, col, true, 1.2f);
            break;
        }
    }
}

static void DrawHeart(ImDrawList* dl, ImVec2 c, float s, ImU32 col, bool filled) {
    const float r  = s * 0.27f;
    const float bx = s * 0.26f;
    const float by = c.y - s * 0.06f;
    const float sx = c.x - s * 0.48f;
    const float sy = by + r * 0.25f;
    const float px = c.x;
    const float py = c.y + s * 0.44f;
    if (filled) {
        dl->AddCircleFilled({c.x - bx, by}, r, col, 12);
        dl->AddCircleFilled({c.x + bx, by}, r, col, 12);
        dl->AddRectFilled({c.x - bx, by - r * 0.6f}, {c.x + bx, by + r * 0.4f}, col);
        ImVec2 tri[3] = {{sx, sy}, {c.x + s * 0.48f, sy}, {px, py}};
        dl->AddTriangleFilled(tri[0], tri[1], tri[2], col);
    } else {
        dl->AddCircle({c.x - bx, by}, r, col, 12, 1.3f);
        dl->AddCircle({c.x + bx, by}, r, col, 12, 1.3f);
        dl->AddLine({sx, sy}, {px, py}, col, 1.3f);
        dl->AddLine({c.x + s * 0.48f, sy}, {px, py}, col, 1.3f);
    }
}

static void DrawCheckmark(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    dl->AddLine({c.x - s * 0.32f, c.y + s * 0.04f},
                {c.x - s * 0.06f, c.y + s * 0.32f}, col, 1.8f);
    dl->AddLine({c.x - s * 0.06f, c.y + s * 0.32f},
                {c.x + s * 0.38f, c.y - s * 0.22f}, col, 1.8f);
}

// ---------------------------------------------------------------------------
// Fish details panel
// ---------------------------------------------------------------------------
static void RenderFishDetails(int fishIdx) {
    if (fishIdx < 0 || fishIdx >= FISH_COUNT) return;
    const Fish& f = FISH_TABLE[fishIdx];

    // Only fetch prices for fillet/contents — fish themselves are not tradeable
    if (f.filletItemId != 0)
        g_Prices.Request(f.filletItemId);

    // Fish icon 48x48
    if (f.itemId != 0) {
        Texture_t* tex = CastAway::IconManager::GetIcon(f.itemId);
        if (tex && tex->Resource) {
            ImGui::Image((ImTextureID)(uintptr_t)tex->Resource, {48.f, 48.f});
        } else {
            CastAway::IconManager::RequestIcon(f.itemId, f.iconUrl ? f.iconUrl : "");
            ImGui::Dummy({48.f, 48.f});
        }
        ImGui::SameLine();
    }

    ImGui::BeginGroup();
    // Name coloured by rarity
    ImGui::TextColored(RarityColor(GetFishRarity(f.itemId)), "%s", f.name);
    if (f.region && f.region[0])
        ImGui::TextDisabled("%s", f.region);
    ImGui::EndGroup();

    ImGui::Separator();

    auto row = [](const char* label, const char* value) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("%s", label);
        ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(value ? value : "-");
    };

    if (ImGui::BeginTable("##FishInfo", 2,
            ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg, {-1.f, 0.f})) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        row("Map",        f.map);
        row("Water",      WaterTypeName(f.water));
        row("Hole",       f.holeType != HoleWater::Any
                              ? HoleWaterName(f.holeType)
                              : "-");
        row("Bait",       BAIT_NAMES[(int)f.bait]);
        row("Time",       TimeOfDayName(f.time));
        row("Collection", f.collection ? f.collection : "-");
        if (g_AchTracker.hoarded) {
            row("Caught", g_AchTracker.IsCaught(fishIdx) ? "Yes" : "No");
        } else {
            row("Caught", "?");
        }
        ImGui::EndTable();
    }

    // Contents / fillet section
    if (f.filletItemId != 0) {
        ImGui::Spacing();
        ImGui::Separator();

        // Icon + name on one line
        Texture_t* ft = CastAway::IconManager::GetIcon(f.filletItemId);
        if (ft && ft->Resource)
            ImGui::Image((ImTextureID)(uintptr_t)ft->Resource, {16.f, 16.f});
        else {
            if (f.filletIconUrl && f.filletIconUrl[0])
                CastAway::IconManager::RequestIcon(f.filletItemId, f.filletIconUrl);
            ImGui::Dummy({16.f, 16.f});
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(f.filletName ? f.filletName : "Contents");

        // TP price for the fillet
        const PriceInfo* fp = g_Prices.Get(f.filletItemId);
        if (fp && fp->fetched) {
            if (fp->tradeable) {
                ImGui::TextDisabled("Buy: ");  ImGui::SameLine(0, 4); RenderCoinPrice(fp->buy_price);
                ImGui::TextDisabled("Sell: "); ImGui::SameLine(0, 4); RenderCoinPrice(fp->sell_price);
            } else {
                ImGui::TextDisabled("  Not tradeable");
            }
        } else if (f.filletItemId != 0) {
            ImGui::TextDisabled("  Fetching...");
        }
    }

    // Bonus drop (e.g. Chunk of Ancient Ambergris)
    if (const BonusItem* bonus = GetBonusItem(f.itemId)) {
        g_Prices.Request(bonus->itemId);
        ImGui::Spacing();
        ImGui::Separator();

        Texture_t* bt = CastAway::IconManager::GetIcon(bonus->itemId);
        if (bt && bt->Resource)
            ImGui::Image((ImTextureID)(uintptr_t)bt->Resource, {16.f, 16.f});
        else {
            if (bonus->iconUrl && bonus->iconUrl[0])
                CastAway::IconManager::RequestIcon(bonus->itemId, bonus->iconUrl);
            ImGui::Dummy({16.f, 16.f});
        }
        ImGui::SameLine();
        if (bonus->isChance) {
            ImGui::TextDisabled("Chance:");
            ImGui::SameLine(0, 4);
        }
        ImGui::TextUnformatted(bonus->name);

        const PriceInfo* bp = g_Prices.Get(bonus->itemId);
        if (bp && bp->fetched && bp->tradeable) {
            ImGui::TextDisabled("Buy: ");  ImGui::SameLine(0, 4); RenderCoinPrice(bp->buy_price);
            ImGui::TextDisabled("Sell: "); ImGui::SameLine(0, 4); RenderCoinPrice(bp->sell_price);
        } else if (!bp || !bp->fetched) {
            ImGui::TextDisabled("  Fetching...");
        }
    }

    // Buttons row
    ImGui::Spacing();
    if (ImGui::SmallButton("Map"))
        g_MapWindowVisible = true;
    if (f.wikiSlug && f.wikiSlug[0]) {
        ImGui::SameLine(0, 6);
        if (ImGui::SmallButton("Open Wiki")) {
            std::string url = std::string("https://wiki.guildwars2.com/wiki/") + f.wikiSlug;
            ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
    }
}

// ---------------------------------------------------------------------------
// Sort helper
// ---------------------------------------------------------------------------
static int RarityRank(const char* r) {
    if (!r) return 0;
    if (!strcmp(r, "Basic"))      return 1;
    if (!strcmp(r, "Fine"))       return 2;
    if (!strcmp(r, "Masterwork")) return 3;
    if (!strcmp(r, "Rare"))       return 4;
    if (!strcmp(r, "Exotic"))     return 5;
    if (!strcmp(r, "Ascended"))   return 6;
    if (!strcmp(r, "Legendary"))  return 7;
    return 0;
}

static void RebuildSortedFishIndices() {
    std::iota(g_SortedFishIndices.begin(), g_SortedFishIndices.end(), 0);
    if (g_SortMode == FishSortMode::Default) return;

    std::sort(g_SortedFishIndices.begin(), g_SortedFishIndices.end(),
        [&](int a, int b) {
            const Fish& fa = FISH_TABLE[a];
            const Fish& fb = FISH_TABLE[b];
            int cmp = 0;
            switch (g_SortMode) {
                case FishSortMode::Alphabetical:
                    cmp = strcmp(fa.name ? fa.name : "", fb.name ? fb.name : "");
                    break;
                case FishSortMode::Rarity: {
                    int ra = RarityRank(GetFishRarity(fa.itemId));
                    int rb = RarityRank(GetFishRarity(fb.itemId));
                    cmp = (ra < rb) ? -1 : (ra > rb) ? 1 : 0;
                    // Tie-break by name so same-rarity fish stay grouped sensibly
                    if (cmp == 0) cmp = strcmp(fa.name ? fa.name : "", fb.name ? fb.name : "");
                    break;
                }
                case FishSortMode::MapName:
                    cmp = strcmp(fa.map ? fa.map : "", fb.map ? fb.map : "");
                    if (cmp == 0) cmp = strcmp(fa.name ? fa.name : "", fb.name ? fb.name : "");
                    break;
                default: break;
            }
            return g_SortAsc ? (cmp < 0) : (cmp > 0);
        });
}

// ---------------------------------------------------------------------------
// RenderFavNotification (and dummy for repositioning)
// ---------------------------------------------------------------------------
static void FavNotifBeginWindow(const char* id, int fishCount, bool locked,
                                bool justUnlocked, ImGuiIO& io) {
    const float W      = 300.f;
    const float lineH  = ImGui::GetTextLineHeightWithSpacing();
    const float btnH   = ImGui::GetFrameHeight();
    const float CARD_H = 66.f;
    const float ROW_H  = CARD_H + btnH + 6.f;  // card + Open Map button + gap
    const float FOOT_H = btnH + 8.f;
    const float HEAD_H = lineH + 6.f;
    const float H      = HEAD_H + (float)fishCount * ROW_H + FOOT_H + 10.f;

    bool posWasReset = (g_FavNotifPos.x < 0.f);
    if (posWasReset)
        g_FavNotifPos = {io.DisplaySize.x / 3.f, io.DisplaySize.y / 3.f + 39.f};

    if (locked || justUnlocked || posWasReset)
        ImGui::SetNextWindowPos(g_FavNotifPos, ImGuiCond_Always);

    ImGui::SetNextWindowSize({W, H}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.88f);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration    |
        ImGuiWindowFlags_NoNav           |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (locked)
        flags |= ImGuiWindowFlags_NoMove;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
    ImGui::Begin(id, nullptr, flags);
}

static void FavNotifEndWindow() {
    if (!g_FavNotifLocked) {
        ImVec2 p = ImGui::GetWindowPos();
        if (p.x != g_FavNotifPos.x || p.y != g_FavNotifPos.y)
            g_FavNotifPos = p;
    }
    ImGui::PopStyleVar();
    ImGui::End();
}

static void RenderFavNotification() {
    if (!g_FavNotifEnabled) return;

    ImGuiIO& io = ImGui::GetIO();
    static bool s_wasLocked = true;
    bool justUnlocked = (s_wasLocked && !g_FavNotifLocked);
    s_wasLocked = g_FavNotifLocked;

    bool realVisible = g_FavNotifActive && !g_FavNotif.dismissed && !g_FavNotif.fish.empty();

    // Auto-dismiss
    if (realVisible && g_AutoDismissSeconds > 0) {
        float age = (float)ImGui::GetTime() - g_FavNotif.createdAt;
        if (age >= (float)g_AutoDismissSeconds) {
            g_FavNotif.dismissed = true;
            realVisible = false;
        }
    }

    if (realVisible) {
        const int MAX_FISH  = 5;
        const int fishCount = (int)std::min((int)g_FavNotif.fish.size(), MAX_FISH);
        FavNotifBeginWindow("##FavNotif", fishCount, g_FavNotifLocked, justUnlocked, io);

        ImGui::TextUnformatted("Favourite fish incoming!");
        ImGui::Separator();

        static const float CARD_H   = 66.f;
        static const float ICON_SZ  = 36.f;
        static const float BORDER_W = 3.f;
        static const float PAD      = 6.f;
        const float lineH2 = ImGui::GetTextLineHeight();
        const float cardW  = ImGui::GetContentRegionAvail().x;
        ImDrawList* dl     = ImGui::GetWindowDrawList();

        int shown = 0;
        for (auto& entry : g_FavNotif.fish) {
            if (shown >= MAX_FISH) break;
            if (entry.fishIdx < 0 || entry.fishIdx >= FISH_COUNT) continue;
            shown++;
            const Fish& f = FISH_TABLE[entry.fishIdx];

            ImVec2 p = ImGui::GetCursorScreenPos();
            ImVec4 rc = RarityColor(GetFishRarity(f.itemId));
            ImU32 rarityCol  = IM_COL32((int)(rc.x*255),(int)(rc.y*255),(int)(rc.z*255),255);
            ImU32 rarityTint = IM_COL32((int)(rc.x*255),(int)(rc.y*255),(int)(rc.z*255), 40);

            // Card background
            dl->AddRectFilled(p, {p.x+cardW, p.y+CARD_H}, IM_COL32(2,20,58,215), 4.f);
            // Rarity gradient wash
            dl->AddRectFilledMultiColor(p, {p.x+cardW*0.6f, p.y+CARD_H},
                                        rarityTint, IM_COL32(0,0,0,0),
                                        IM_COL32(0,0,0,0), rarityTint);
            // Rarity border
            dl->AddRectFilled(p, {p.x+BORDER_W, p.y+CARD_H}, rarityCol, 4.f,
                              ImDrawCornerFlags_Left);
            // Outline
            dl->AddRect(p, {p.x+cardW, p.y+CARD_H}, IM_COL32(20,60,110,180), 4.f);

            // Icon
            float ix = p.x + BORDER_W + PAD;
            float iy = p.y + (CARD_H - ICON_SZ) * 0.5f;
            if (f.itemId != 0) {
                Texture_t* tex = CastAway::IconManager::GetIcon(f.itemId);
                if (tex && tex->Resource)
                    dl->AddImage((ImTextureID)(uintptr_t)tex->Resource,
                                 {ix, iy}, {ix+ICON_SZ, iy+ICON_SZ});
                else {
                    CastAway::IconManager::RequestIcon(f.itemId, f.iconUrl ? f.iconUrl : "");
                    dl->AddRectFilled({ix,iy},{ix+ICON_SZ,iy+ICON_SZ}, IM_COL32(30,30,30,200), 3.f);
                }
            }

            // Text
            float tx = ix + ICON_SZ + PAD;
            uint32_t secs = SecondsUntilPhase(f.time);
            dl->AddText({tx, p.y+PAD}, rarityCol, f.name);
            dl->AddText({tx, p.y+PAD+lineH2+2.f}, IM_COL32(130,130,130,255),
                        f.map ? f.map : "?");
            char timeBuf[48];
            snprintf(timeBuf, sizeof(timeBuf), "%s in %um %02us",
                     TimeOfDayName(f.time), secs/60, secs%60);
            dl->AddText({tx, p.y+PAD+lineH2*2.f+4.f}, IM_COL32(160,160,160,220), timeBuf);

            ImGui::Dummy({cardW, CARD_H});

            if (entry.mapId != 0) {
                char btnLbl[64];
                snprintf(btnLbl, sizeof(btnLbl), "Open Map: %s##om%d",
                         f.map ? f.map : "?", entry.fishIdx);
                if (ImGui::Button(btnLbl)) {
                    g_MapWindowVisible = true;
                    g_MapPanel.NavigateToMap(entry.mapId);
                }
            }
            ImGui::Spacing();
        }
        if ((int)g_FavNotif.fish.size() > MAX_FISH)
            ImGui::TextDisabled("...and %d more", (int)g_FavNotif.fish.size() - MAX_FISH);

        ImGui::Separator();
        float btnW = 80.f;
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - btnW - ImGui::GetStyle().WindowPadding.x);
        if (ImGui::Button("Dismiss##favnotif", {btnW, 0.f}))
            g_FavNotif.dismissed = true;

        FavNotifEndWindow();

    } else if (!g_FavNotifLocked) {
        // Dummy panel for repositioning
        FavNotifBeginWindow("##FavNotifDummy", 1, false, justUnlocked, io);
        ImGui::TextUnformatted("Favourite fish incoming!");
        ImGui::Separator();
        {
            static const float CARD_H   = 66.f;
            static const float ICON_SZ  = 36.f;
            static const float BORDER_W = 3.f;
            static const float PAD      = 6.f;
            const float lineH2 = ImGui::GetTextLineHeight();
            const float cardW  = ImGui::GetContentRegionAvail().x;
            ImDrawList* dl     = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetCursorScreenPos();
            // Rare (gold) dummy card
            ImU32 rarityCol  = IM_COL32(252,209,11,255);
            ImU32 rarityTint = IM_COL32(252,209,11, 40);
            dl->AddRectFilled(p, {p.x+cardW, p.y+CARD_H}, IM_COL32(2,20,58,215), 4.f);
            dl->AddRectFilledMultiColor(p, {p.x+cardW*0.6f, p.y+CARD_H},
                                        rarityTint, IM_COL32(0,0,0,0),
                                        IM_COL32(0,0,0,0), rarityTint);
            dl->AddRectFilled(p, {p.x+BORDER_W, p.y+CARD_H}, rarityCol, 4.f,
                              ImDrawCornerFlags_Left);
            dl->AddRect(p, {p.x+cardW, p.y+CARD_H}, IM_COL32(20,60,110,180), 4.f);
            float ix = p.x + BORDER_W + PAD;
            float iy = p.y + (CARD_H - ICON_SZ) * 0.5f;
            dl->AddRectFilled({ix,iy},{ix+ICON_SZ,iy+ICON_SZ}, IM_COL32(30,30,30,200), 3.f);
            float tx = ix + ICON_SZ + PAD;
            dl->AddText({tx, p.y+PAD}, rarityCol, "Shining Dragonfish");
            dl->AddText({tx, p.y+PAD+lineH2+2.f},  IM_COL32(130,130,130,255), "Cantha");
            dl->AddText({tx, p.y+PAD+lineH2*2.f+4.f}, IM_COL32(160,160,160,220), "Dawn in 2m 30s");
            ImGui::Dummy({cardW, CARD_H});
        }
        ImGui::Button("Open Map: Cantha##omdummy");
        ImGui::Spacing();
        ImGui::Separator();
        float btnW = 80.f;
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - btnW - ImGui::GetStyle().WindowPadding.x);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.4f);
        ImGui::Button("Dismiss##favdummy", {btnW, 0.f});
        ImGui::PopStyleVar();
        FavNotifEndWindow();
    }
}

// ---------------------------------------------------------------------------
// AddonRender
// ---------------------------------------------------------------------------
void AddonRender() {
    // --- MumbleLink ---
    int   mumbleMapId = 0;
    float game_x = 0.f, game_z = 0.f;

    MumbleLinkedMem* mumble =
        reinterpret_cast<MumbleLinkedMem*>(APIDefs->DataLink_Get(DL_MUMBLE_LINK));
    if (mumble && mumble->uiTick > 0) {
        const GW2Context* ctx =
            reinterpret_cast<const GW2Context*>(mumble->context);
        uint32_t rawMapId = ctx->mapId;
        // GW2 map IDs are well under 100k. Anything larger is stale/garbage memory.
        mumbleMapId = (rawMapId > 0 && rawMapId < 100000) ? (int)rawMapId : 0;
        game_x = mumble->fAvatarPosition[0];
        game_z = mumble->fAvatarPosition[2];

        if (mumbleMapId != 0) {
            for (int i = 0; i < HOLE_COUNT; i++) {
                if ((int)HOLE_TABLE[i].mapId == mumbleMapId && HOLE_TABLE[i].map) {
                    g_CurrentMapName = HOLE_TABLE[i].map;
                    break;
                }
            }
        }
    }

    // --- Per-frame work ---
    g_AchTracker.FlushPendingQuery();
    g_MapPanel.ProcessReadyQueue();
    CastAway::IconManager::Tick();

    UpdateHoleDwellTimers(mumbleMapId, game_x, game_z);
    CheckTimeWindowNotifications();
    CheckNearbyHoles(mumbleMapId, game_x, game_z);

    RenderOverlay();
    RenderToasts();
    RenderFavNotification();

    // --- Map floating window (independent of main window visibility) ---
    if (g_MapWindowVisible) {
        ImGui::SetNextWindowSize({640.f, 520.f}, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Cast Away — Map##CastAwayMapWindow", &g_MapWindowVisible,
                         ImGuiWindowFlags_NoScrollbar)) {
            if (g_LastDetailFish < 0)
                ImGui::TextDisabled("Select a fish in the database to show its fishing holes.");
            else
                g_MapPanel.Render(g_LastDetailFish, mumbleMapId, game_x, game_z);
        }
        ImGui::End();
    }

    if (!g_WindowVisible) return;

    // --- Main window ---
    ImGui::SetNextWindowSize({800.f, 560.f}, ImGuiCond_FirstUseEver);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.07f, 0.15f, 1.f));
    if (!ImGui::Begin("Cast Away##MainWindow", &g_WindowVisible,
                      ImGuiWindowFlags_NoScrollbar)) {
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }
    ImGui::PopStyleColor();

    float contentWidth = ImGui::GetContentRegionAvail().x;
    RenderDayNightBar(contentWidth);
    ImGui::Spacing();

    // Tab navigation from Favourites → Database
    static int forceTab = -1;
    if (g_SwitchToDatabase) {
        forceTab = 0;
        g_SwitchToDatabase = false;
    }

    // Shared card layout constants (used by Database, Favourites, Achievements tabs)
    static const float cardGap  = 4.f;
    static const float cardH    = 66.f;
    static const float iconSz   = 36.f;
    static const float borderSz = 3.f;
    static const float pad      = 6.f;

    if (ImGui::BeginTabBar("##CastAwayTabs")) {

        // ===== DATABASE TAB =====
        ImGuiTabItemFlags dbFlags = (forceTab == 0) ? ImGuiTabItemFlags_SetSelected : 0;
        if (forceTab == 0) forceTab = -1;
        if (ImGui::BeginTabItem("Database", nullptr, dbFlags)) {

            // Filter bar
            ImGui::SetNextItemWidth(140.f);
            ImGui::InputText("##Search", g_SearchBuf, sizeof(g_SearchBuf));
            ImGui::SameLine();

            ImGui::SetNextItemWidth(110.f);
            if (ImGui::BeginCombo("##Bait",
                    g_FilterBait == 0 ? "All Bait" : BAIT_NAMES[g_FilterBait])) {
                if (ImGui::Selectable("All Bait", g_FilterBait == 0)) g_FilterBait = 0;
                for (int i = 1; i < BAIT_COUNT; i++) {
                    if (i == (int)BaitType::BorrowedBait) continue;
                    if (const BaitInfo* bi = GetBaitInfo((BaitType)i)) {
                        Texture_t* tex = CastAway::IconManager::GetIcon(bi->itemId);
                        if (tex && tex->Resource)
                            ImGui::Image((ImTextureID)(uintptr_t)tex->Resource, {14.f, 14.f});
                        else {
                            CastAway::IconManager::RequestIcon(bi->itemId, bi->iconUrl);
                            ImGui::Dummy({14.f, 14.f});
                        }
                        ImGui::SameLine(0, 4);
                    }
                    if (ImGui::Selectable(BAIT_NAMES[i], g_FilterBait == i)) g_FilterBait = i;
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();

            static const char* timeOpts[] = {"All Times","Dawn","Day","Dusk","Night"};
            const char* timeLbl = g_ShowCurrentOnly ? "Now"
                                : timeOpts[g_FilterTime < 0 ? 0 : g_FilterTime];
            ImGui::SetNextItemWidth(90.f);
            if (ImGui::BeginCombo("##Time", timeLbl)) {
                if (ImGui::Selectable("All Times", !g_ShowCurrentOnly && g_FilterTime == 0)) {
                    g_ShowCurrentOnly = false; g_FilterTime = 0;
                }
                for (int i = 1; i <= 4; i++) {
                    if (ImGui::Selectable(timeOpts[i], !g_ShowCurrentOnly && g_FilterTime == i)) {
                        g_ShowCurrentOnly = false; g_FilterTime = i;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();

            if (ImGui::SmallButton("Now")) {
                g_ShowCurrentOnly = true;
                g_FilterMap = g_CurrentMapName;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear")) {
                g_SearchBuf[0]    = '\0';
                g_FilterBait      = 0;
                g_FilterTime      = 0;
                g_ShowCurrentOnly = false;
                g_MissingOnly     = false;
                g_FilterMap.clear();
            }
            ImGui::SameLine();
            ImGui::Checkbox("Missing only", &g_MissingOnly);
            ImGui::SameLine();

            // Sort dropdown
            static const char* sortLabels[] = { "Default", "A–Z", "Rarity", "Map" };
            ImGui::SetNextItemWidth(90.f);
            if (ImGui::BeginCombo("##Sort", sortLabels[(int)g_SortMode])) {
                for (int i = 0; i < 4; ++i) {
                    if (ImGui::Selectable(sortLabels[i], (int)g_SortMode == i)) {
                        g_SortMode = (FishSortMode)i;
                        g_SortDirty = true;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();

            // Direction arrow button
            if (ImGui::SmallButton(g_SortAsc ? "v" : "^")) {
                g_SortAsc = !g_SortAsc;
                g_SortDirty = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(g_SortAsc ? "Ascending — click to reverse"
                                            : "Descending — click to reverse");

            if (g_SortDirty) {
                RebuildSortedFishIndices();
                g_SortDirty = false;
            }

            ImGui::Spacing();

            // Split layout
            const float detailsW = 260.f;
            float tableW = contentWidth - detailsW - 8.f;
            float tableH   = ImGui::GetContentRegionAvail().y - 4.f;

            // Card grid — two columns
            const float lineH = ImGui::GetTextLineHeight();

            // Full swim bounds span card area + details pane; each child clips to its own rect
            ImVec2 creatureMn = ImGui::GetCursorScreenPos();
            ImVec2 creatureMx = {creatureMn.x + tableW + 8.f + detailsW, creatureMn.y + tableH};

            if (ImGui::BeginChild("##FishCards", {tableW, tableH}, false)) {
                const float cardW = floorf((ImGui::GetContentRegionAvail().x - cardGap) / 2.f);
                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImVec2 tankMn = ImGui::GetWindowPos();
                ImVec2 tankMx = {tankMn.x + ImGui::GetWindowWidth(), tankMn.y + ImGui::GetWindowHeight()};
                DrawFishtankBg(dl, tankMn, tankMx);
                DrawFishtankCreatures(dl, creatureMn, creatureMx);
                int col = 0;

                for (int idx : g_SortedFishIndices) {
                    if (!FishMatchesFilter(idx)) continue;
                    const Fish& f = FISH_TABLE[idx];

                    if (col == 1)
                        ImGui::SameLine(cardW + cardGap);

                    ImVec2 p = ImGui::GetCursorScreenPos();

                    // Full-card invisible button drives selection
                    char cardId[32];
                    snprintf(cardId, sizeof(cardId), "##card%d", idx);
                    ImGui::InvisibleButton(cardId, {cardW, cardH});
                    bool cardClicked = ImGui::IsItemClicked();
                    bool hovered     = ImGui::IsItemHovered();

                    // Heart hit-test (intercepts clicks before card selection)
                    float hx = p.x + cardW - 14.f;
                    float hy = p.y + 14.f;
                    bool heartHit = ImGui::IsMouseHoveringRect({hx-10.f, hy-10.f},
                                                               {hx+10.f, hy+10.f});
                    bool heartClicked = heartHit && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
                    if (heartClicked)       ToggleFavourite(f.name);
                    else if (cardClicked) { g_SelectedFish = idx; g_LastDetailFish = idx; }

                    // Card background
                    ImU32 bgCol = (g_SelectedFish == idx)
                                  ? IM_COL32( 5, 45,110, 245)
                                  : hovered ? IM_COL32( 3, 32, 82, 230)
                                            : IM_COL32( 2, 20, 58, 215);
                    dl->AddRectFilled(p, {p.x + cardW, p.y + cardH}, bgCol, 4.f);

                    // Rarity border (left edge)
                    ImVec4 rc = RarityColor(GetFishRarity(f.itemId));
                    ImU32 rarityCol = IM_COL32((int)(rc.x*255), (int)(rc.y*255),
                                               (int)(rc.z*255), 255);
                    // Rarity gradient wash: rarity colour at left fading to transparent
                    ImU32 rarityTint = IM_COL32((int)(rc.x*255),(int)(rc.y*255),(int)(rc.z*255), 40);
                    dl->AddRectFilledMultiColor(p, {p.x + cardW*0.6f, p.y + cardH},
                                               rarityTint, IM_COL32(0,0,0,0),
                                               IM_COL32(0,0,0,0), rarityTint);
                    dl->AddRectFilled(p, {p.x + borderSz, p.y + cardH},
                                     rarityCol, 4.f, ImDrawCornerFlags_Left);

                    // Card outline
                    ImU32 outlineCol = (g_SelectedFish == idx)
                                       ? IM_COL32(90, 143, 212, 200)
                                       : IM_COL32(20, 60,110, 180);
                    dl->AddRect(p, {p.x + cardW, p.y + cardH}, outlineCol, 4.f);

                    // Fish icon (36x36, vertically centred)
                    float ix = p.x + borderSz + pad;
                    float iy = p.y + (cardH - iconSz) * 0.5f;
                    if (f.itemId != 0) {
                        Texture_t* tex = CastAway::IconManager::GetIcon(f.itemId);
                        if (tex && tex->Resource)
                            dl->AddImage((ImTextureID)(uintptr_t)tex->Resource,
                                        {ix, iy}, {ix + iconSz, iy + iconSz});
                        else {
                            CastAway::IconManager::RequestIcon(f.itemId,
                                                               f.iconUrl ? f.iconUrl : "");
                            dl->AddRectFilled({ix, iy}, {ix+iconSz, iy+iconSz},
                                             IM_COL32(30,30,30,200), 3.f);
                        }
                    }

                    // Text block
                    float tx         = ix + iconSz + pad;
                    float tRightEdge = p.x + cardW - 26.f; // leave room for heart/dot column

                    // Name in rarity colour
                    ImU32 nameCol = IM_COL32((int)(rc.x*255),(int)(rc.y*255),(int)(rc.z*255),255);
                    dl->AddText({tx, p.y + pad}, nameCol, f.name);

                    // Map (dimmed)
                    dl->AddText({tx, p.y + pad + lineH + 2.f},
                               IM_COL32(130,130,130,255), f.map ? f.map : "");

                    // Attribute chips: bait · time · water
                    float chipY = p.y + pad + lineH * 2.f + 5.f;
                    float chipX = tx;
                    auto chip = [&](const char* text, ImU32 iconCol, auto drawIcon) {
                        ImVec2 tsz = ImGui::CalcTextSize(text);
                        float iconW = tsz.y;
                        float cw = 3.f+iconW+3.f+tsz.x+3.f, ch = tsz.y+2.f;
                        if (chipX + cw > tRightEdge) return;
                        dl->AddRectFilled({chipX,chipY},{chipX+cw,chipY+ch},
                                         IM_COL32(4,16,40,220), 2.f);
                        drawIcon(dl, {chipX+3.f,chipY+1.f}, iconW, iconCol);
                        dl->AddText({chipX+3.f+iconW+3.f,chipY+1.f},
                                   IM_COL32(153,153,153,255), text);
                        chipX += cw + 3.f;
                    };
                    chip(BAIT_NAMES[(int)f.bait], IM_COL32(200,160,60,255),
                        [&](ImDrawList* d, ImVec2 ip, float s, ImU32 c) {
                            if (const BaitInfo* bi = GetBaitInfo(f.bait)) {
                                Texture_t* tx = CastAway::IconManager::GetIcon(bi->itemId);
                                if (tx && tx->Resource) {
                                    d->AddImage((ImTextureID)(uintptr_t)tx->Resource,
                                               {ip.x, ip.y}, {ip.x+s, ip.y+s});
                                    return;
                                }
                                CastAway::IconManager::RequestIcon(bi->itemId, bi->iconUrl);
                            }
                            DrawChipBaitIcon(d, ip, s, c);
                        });
                    chip(TimeOfDayName(f.time), ChipTimeColor(f.time),
                        [&](ImDrawList* d, ImVec2 ip, float s, ImU32 c){
                            DrawChipTimeIcon(d, ip, s, f.time, c); });
                    chip(WaterTypeName(f.water), ChipWaterColor(f.water),
                        [&](ImDrawList* d, ImVec2 ip, float s, ImU32 c){
                            DrawChipWaterIcon(d, ip, s, f.water, c); });

                    // Heart (solid — red if fav, grey if not)
                    bool fav = IsFavourite(f.name);
                    DrawHeart(dl, {hx, hy}, 11.f,
                             fav ? IM_COL32(210, 50, 50, 255)
                                 : IM_COL32(110, 110, 110, 200), true);

                    // Caught dot (below heart)
                    bool caught = g_AchTracker.hoarded && g_AchTracker.IsCaught(idx);
                    dl->AddCircleFilled({hx, hy + 18.f}, 4.f,
                                       caught ? IM_COL32(76,175,80,255)
                                              : IM_COL32(55,55,55,255));

                    col = 1 - col;
                }
            }
            ImGui::EndChild();

            // Details panel — ocean-tinted background, fish swim behind it too
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.08f, 0.23f, 0.97f));
            if (ImGui::BeginChild("##FishDetailsPanel", {detailsW, tableH}, true)) {
                RenderFishDetails(g_LastDetailFish);
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();

            ImGui::EndTabItem();
        }

        // ===== FAVOURITES TAB =====
        if (ImGui::BeginTabItem("Favourites")) {
            float favH = ImGui::GetContentRegionAvail().y - 4.f;
            ImVec2 favCreMn = ImGui::GetCursorScreenPos();
            ImVec2 favCreMx = {favCreMn.x + ImGui::GetContentRegionAvail().x, favCreMn.y + favH};

            if (ImGui::BeginChild("##FavCards", {-1.f, favH}, false)) {
                ImDrawList* dl  = ImGui::GetWindowDrawList();
                ImVec2 fTankMn = ImGui::GetWindowPos();
                ImVec2 fTankMx = {fTankMn.x + ImGui::GetWindowWidth(), fTankMn.y + ImGui::GetWindowHeight()};
                DrawFishtankBg(dl, fTankMn, fTankMx);
                DrawFishtankCreatures(dl, favCreMn, favCreMx);
                const float fLineH = ImGui::GetTextLineHeight();
                const float fGap   = cardGap;
                const float fCardW = floorf((ImGui::GetContentRegionAvail().x - fGap) / 2.f);
                int fcol = 0;

                for (int i = 0; i < FISH_COUNT; i++) {
                    const Fish& f = FISH_TABLE[i];
                    if (!IsFavourite(f.name)) continue;

                    if (fcol == 1) ImGui::SameLine(fCardW + fGap);

                    ImVec2 p = ImGui::GetCursorScreenPos();
                    char cardId[32];
                    snprintf(cardId, sizeof(cardId), "##fcard%d", i);
                    ImGui::InvisibleButton(cardId, {fCardW, cardH});
                    bool cardClicked = ImGui::IsItemClicked();
                    bool hovered     = ImGui::IsItemHovered();

                    float hx = p.x + fCardW - 14.f, hy = p.y + 14.f;
                    bool heartHit     = ImGui::IsMouseHoveringRect({hx-10.f,hy-10.f},{hx+10.f,hy+10.f});
                    bool heartClicked = heartHit && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
                    if (heartClicked) ToggleFavourite(f.name);
                    else if (cardClicked) {
                        g_SelectedFish = i; g_LastDetailFish = i; g_SwitchToDatabase = true;
                    }

                    ImU32 bgCol = hovered ? IM_COL32(3, 32, 82, 230) : IM_COL32(2, 20, 58, 215);
                    dl->AddRectFilled(p, {p.x+fCardW, p.y+cardH}, bgCol, 4.f);

                    ImVec4 rc = RarityColor(GetFishRarity(f.itemId));
                    ImU32 rarityCol = IM_COL32((int)(rc.x*255),(int)(rc.y*255),(int)(rc.z*255),255);
                    ImU32 rarityTint = IM_COL32((int)(rc.x*255),(int)(rc.y*255),(int)(rc.z*255), 40);
                    dl->AddRectFilledMultiColor(p, {p.x+fCardW*0.6f, p.y+cardH},
                                               rarityTint, IM_COL32(0,0,0,0),
                                               IM_COL32(0,0,0,0), rarityTint);
                    dl->AddRectFilled(p, {p.x+borderSz, p.y+cardH}, rarityCol, 4.f, ImDrawCornerFlags_Left);
                    dl->AddRect(p, {p.x+fCardW, p.y+cardH}, IM_COL32(20,60,110,180), 4.f);

                    float ix = p.x+borderSz+pad, iy = p.y+(cardH-iconSz)*0.5f;
                    if (f.itemId != 0) {
                        Texture_t* tex = CastAway::IconManager::GetIcon(f.itemId);
                        if (tex && tex->Resource)
                            dl->AddImage((ImTextureID)(uintptr_t)tex->Resource,
                                        {ix,iy},{ix+iconSz,iy+iconSz});
                        else {
                            CastAway::IconManager::RequestIcon(f.itemId, f.iconUrl ? f.iconUrl : "");
                            dl->AddRectFilled({ix,iy},{ix+iconSz,iy+iconSz},IM_COL32(30,30,30,200),3.f);
                        }
                    }

                    float tx = ix+iconSz+pad, tRightEdge = p.x+fCardW-26.f;
                    dl->AddText({tx, p.y+pad}, rarityCol, f.name);
                    dl->AddText({tx, p.y+pad+fLineH+2.f}, IM_COL32(130,130,130,255), f.map ? f.map : "");

                    float chipY = p.y+pad+fLineH*2.f+5.f, chipX = tx;
                    auto chip = [&](const char* text, ImU32 iconCol, auto drawIcon) {
                        ImVec2 tsz = ImGui::CalcTextSize(text);
                        float iconW = tsz.y, cw = 3.f+iconW+3.f+tsz.x+3.f, ch = tsz.y+2.f;
                        if (chipX + cw > tRightEdge) return;
                        dl->AddRectFilled({chipX,chipY},{chipX+cw,chipY+ch},IM_COL32(34,34,34,255),2.f);
                        drawIcon(dl, {chipX+3.f,chipY+1.f}, iconW, iconCol);
                        dl->AddText({chipX+3.f+iconW+3.f,chipY+1.f},IM_COL32(153,153,153,255),text);
                        chipX += cw + 3.f;
                    };
                    chip(BAIT_NAMES[(int)f.bait], IM_COL32(200,160,60,255),
                        [&](ImDrawList* d, ImVec2 ip, float s, ImU32 c) {
                            if (const BaitInfo* bi = GetBaitInfo(f.bait)) {
                                Texture_t* tx = CastAway::IconManager::GetIcon(bi->itemId);
                                if (tx && tx->Resource) {
                                    d->AddImage((ImTextureID)(uintptr_t)tx->Resource,
                                               {ip.x, ip.y}, {ip.x+s, ip.y+s});
                                    return;
                                }
                                CastAway::IconManager::RequestIcon(bi->itemId, bi->iconUrl);
                            }
                            DrawChipBaitIcon(d, ip, s, c);
                        });
                    chip(TimeOfDayName(f.time), ChipTimeColor(f.time),
                        [&](ImDrawList* d, ImVec2 ip, float s, ImU32 c){
                        DrawChipTimeIcon(d, ip, s, f.time, c); });
                    chip(WaterTypeName(f.water), ChipWaterColor(f.water),
                        [&](ImDrawList* d, ImVec2 ip, float s, ImU32 c){
                            DrawChipWaterIcon(d, ip, s, f.water, c); });

                    DrawHeart(dl, {hx,hy}, 11.f, IM_COL32(210,50,50,255), true);
                    bool caught = g_AchTracker.hoarded && g_AchTracker.IsCaught(i);
                    dl->AddCircleFilled({hx, hy+18.f}, 4.f,
                                       caught ? IM_COL32(76,175,80,255) : IM_COL32(55,55,55,255));

                    fcol = 1 - fcol;
                }
            }
            ImGui::EndChild();

            ImGui::EndTabItem();
        }

        // ===== MAP TAB =====
        // ===== ACHIEVEMENTS TAB =====
        if (ImGui::BeginTabItem("Collections")) {
            if (!g_AchTracker.hoarded) {
                ImGui::TextWrapped("Requires the Hoard & Seek addon to be installed and configured.");
                ImGui::TextDisabled("Install Hoard & Seek, then restart GW2.");
            } else {
                for (int ci = 0; ci < COLLECTION_COUNT; ++ci) {
                    const FishingCollection& col = COLLECTION_TABLE[ci];
                    if (col.achievementId == 0) continue;
                    const CollectionState* st = g_AchTracker.GetCollection(col.achievementId);
                    if (!st) continue;

                    // Achievement icon (20x20)
                    if (col.iconUrl && col.iconUrl[0]) {
                        uint32_t iconKey = col.achievementId + 9000000u;
                        Texture_t* ach_icon = CastAway::IconManager::GetIcon(iconKey);
                        if (ach_icon && ach_icon->Resource)
                            ImGui::Image(ach_icon->Resource, {20, 20});
                        else {
                            CastAway::IconManager::RequestIcon(iconKey, col.iconUrl);
                            ImGui::Dummy({20, 20});
                        }
                        ImGui::SameLine();
                    }

                    char header[128];
                    snprintf(header, sizeof(header), "%s  %u/%u##achcol%u",
                             col.name, st->caughtCount, st->totalFish, col.achievementId);
                    bool open = ImGui::CollapsingHeader(header);

                    // Progress bar — render after CollapsingHeader
                    float frac = st->totalFish > 0 ? (float)st->caughtCount / st->totalFish : 0.f;
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 160.f);
                    ImGui::SetNextItemWidth(150.f);
                    ImGui::ProgressBar(frac, {150.f, ImGui::GetTextLineHeight()});

                    if (!open) continue;

                    ImGui::Indent();
                    {
                        const float achGap    = 4.f;
                        const float achCardH  = 62.f;
                        const float achIconSz = 24.f;
                        const float achStartX = ImGui::GetCursorPosX();
                        const float achW = floorf((ImGui::GetContentRegionAvail().x - achGap * 2.f) / 3.f);
                        const float achLH = ImGui::GetTextLineHeight();
                        int achCol = 0;

                        for (int fi = 0; fi < FISH_COUNT; ++fi) {
                            const Fish& f = FISH_TABLE[fi];
                            if (f.achievementId != col.achievementId) continue;

                            bool caught = st->done || (st->bitsKnown &&
                                          f.bitIndex < 64 && st->caughtBits[f.bitIndex]);

                            if (achCol > 0) ImGui::SameLine(achStartX + achCol * (achW + achGap));

                            ImVec2 ap = ImGui::GetCursorScreenPos();
                            char achId[32];
                            snprintf(achId, sizeof(achId), "##achcard%d", fi);
                            ImGui::InvisibleButton(achId, {achW, achCardH});
                            if (ImGui::IsItemClicked()) {
                                g_SelectedFish = fi; g_LastDetailFish = fi;
                                g_SwitchToDatabase = true;
                            }
                            bool achHov = ImGui::IsItemHovered();

                            ImDrawList* adl = ImGui::GetWindowDrawList();
                            ImU32 achBg = achHov ? IM_COL32(3,32,82,230) : IM_COL32(2,20,58,215);
                            adl->AddRectFilled(ap, {ap.x+achW, ap.y+achCardH}, achBg, 4.f);

                            ImVec4 arc = RarityColor(GetFishRarity(f.itemId));
                            ImU32 aRar = IM_COL32((int)(arc.x*255),(int)(arc.y*255),(int)(arc.z*255),255);
                            ImU32 aRarTint = IM_COL32((int)(arc.x*255),(int)(arc.y*255),(int)(arc.z*255),40);
                            adl->AddRectFilledMultiColor(ap, {ap.x+achW*0.6f, ap.y+achCardH},
                                                         aRarTint, IM_COL32(0,0,0,0),
                                                         IM_COL32(0,0,0,0), aRarTint);
                            adl->AddRectFilled(ap, {ap.x+3.f, ap.y+achCardH}, aRar, 4.f, ImDrawCornerFlags_Left);
                            adl->AddRect(ap, {ap.x+achW, ap.y+achCardH}, IM_COL32(20,60,110,180), 4.f);

                            const float acPad = 4.f;
                            float aix = ap.x + 3.f + acPad, aiy = ap.y + acPad;
                            if (f.itemId != 0) {
                                Texture_t* ti = CastAway::IconManager::GetIcon(f.itemId);
                                if (ti && ti->Resource)
                                    adl->AddImage((ImTextureID)(uintptr_t)ti->Resource,
                                                 {aix,aiy},{aix+achIconSz,aiy+achIconSz});
                                else {
                                    CastAway::IconManager::RequestIcon(f.itemId, f.iconUrl ? f.iconUrl : "");
                                    adl->AddRectFilled({aix,aiy},{aix+achIconSz,aiy+achIconSz},IM_COL32(30,30,30,200),3.f);
                                }
                            }

                            const float acRightX = ap.x + achW - 18.f;
                            float atx = aix + achIconSz + 4.f;
                            adl->AddText({atx, ap.y + acPad},              aRar,                       f.name);
                            adl->AddText({atx, ap.y + acPad + achLH + 2.f}, IM_COL32(130,130,130,255), f.map ? f.map : "");

                            // Bait · time · water chips
                            float acChipY = ap.y + acPad + achLH * 2.f + 5.f;
                            float acChipX = atx;
                            auto acChip = [&](const char* text, ImU32 iconCol, auto drawIcon) {
                                ImVec2 tsz = ImGui::CalcTextSize(text);
                                float iw = tsz.y;
                                float cw = 3.f+iw+3.f+tsz.x+3.f, ch = tsz.y+2.f;
                                if (acChipX + cw > acRightX) return;
                                adl->AddRectFilled({acChipX,acChipY},{acChipX+cw,acChipY+ch},IM_COL32(4,16,40,220),2.f);
                                drawIcon(adl, {acChipX+3.f,acChipY+1.f}, iw, iconCol);
                                adl->AddText({acChipX+3.f+iw+3.f,acChipY+1.f},IM_COL32(153,153,153,255),text);
                                acChipX += cw + 3.f;
                            };
                            acChip(BAIT_NAMES[(int)f.bait], IM_COL32(200,160,60,255),
                                [&](ImDrawList* d, ImVec2 ip, float s, ImU32 c) {
                                    if (const BaitInfo* bi = GetBaitInfo(f.bait)) {
                                        Texture_t* bx = CastAway::IconManager::GetIcon(bi->itemId);
                                        if (bx && bx->Resource) {
                                            d->AddImage((ImTextureID)(uintptr_t)bx->Resource,{ip.x,ip.y},{ip.x+s,ip.y+s});
                                            return;
                                        }
                                        CastAway::IconManager::RequestIcon(bi->itemId, bi->iconUrl);
                                    }
                                    DrawChipBaitIcon(d, ip, s, c);
                                });
                            acChip(TimeOfDayName(f.time), ChipTimeColor(f.time),
                                [&](ImDrawList* d, ImVec2 ip, float s, ImU32 c){ DrawChipTimeIcon(d,ip,s,f.time,c); });
                            acChip(WaterTypeName(f.water), ChipWaterColor(f.water),
                                [&](ImDrawList* d, ImVec2 ip, float s, ImU32 c){ DrawChipWaterIcon(d,ip,s,f.water,c); });

                            // Heart + caught dot (right column)
                            const float acRx = ap.x + achW - 9.f;
                            bool fav = IsFavourite(f.name);
                            DrawHeart(adl, {acRx, ap.y + achCardH * 0.5f - 10.f}, 9.f,
                                      fav ? IM_COL32(210,50,50,255) : IM_COL32(110,110,110,200), true);
                            adl->AddCircleFilled({acRx, ap.y + achCardH * 0.5f + 6.f}, 4.f,
                                               caught ? IM_COL32(76,175,80,255) : IM_COL32(55,55,55,255));

                            if (++achCol >= 3) achCol = 0;
                        }
                    }
                    ImGui::Unindent();
                }
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();

}

// ---------------------------------------------------------------------------
// AddonOptions
// ---------------------------------------------------------------------------
void AddonOptions() {
    ImGui::Text("Cast Away v%d.%d", V_MAJOR, V_MINOR);
    ImGui::Separator();

    bool prevQA = g_ShowQAIcon;
    ImGui::Checkbox("Show quick access icon", &g_ShowQAIcon);
    if (g_ShowQAIcon != prevQA) {
        if (g_ShowQAIcon)
            APIDefs->QuickAccess_Add(QA_ID, TEX_ICON, TEX_ICON_HOVER,
                                     "KB_CAST_AWAY_TOGGLE", "Cast Away");
        else
            APIDefs->QuickAccess_Remove(QA_ID);
    }

    ImGui::Separator();

    ImGui::Checkbox("Show time overlay", &g_OverlayVisible);
    if (g_OverlayVisible) {
        ImGui::Indent();
        ImGui::Checkbox("Lock position##overlay", &g_OverlayLocked);
        ImGui::SameLine();
        if (ImGui::Button("Reset position##overlay"))
            g_OverlayPos = {-1.f, -1.f};
        ImGui::Unindent();
    }

    ImGui::Separator();

    ImGui::Checkbox("Notify when favourite fish can be caught", &g_FavNotifEnabled);
    if (g_FavNotifEnabled) {
        ImGui::Indent();

        int mins = g_NotifyLeadSeconds / 60;
        if (ImGui::SliderInt("Time before##notif", &mins, 1, 15, "%d min"))
            g_NotifyLeadSeconds = mins * 60;

        // Auto-close: index 0 = never, 1-24 = 5-120 seconds in 5s steps
        int autoIdx = (g_AutoDismissSeconds == 0) ? 0
                    : std::clamp(g_AutoDismissSeconds / 5, 1, 24);
        char autoFmt[16];
        snprintf(autoFmt, sizeof(autoFmt),
                 autoIdx == 0 ? "Never" : "%d sec", autoIdx * 5);
        if (ImGui::SliderInt("Auto close##notif", &autoIdx, 0, 24, autoFmt))
            g_AutoDismissSeconds = (autoIdx == 0) ? 0 : autoIdx * 5;

        ImGui::Checkbox("Lock position##favnotif", &g_FavNotifLocked);
        ImGui::SameLine();
        if (ImGui::Button("Reset position##favnotif"))
            g_FavNotifPos = {-1.f, -1.f};

        ImGui::Unindent();
    }
    ImGui::Spacing();

}

// ---------------------------------------------------------------------------
// GetAddonDef
// ---------------------------------------------------------------------------
extern "C" __declspec(dllexport) AddonDefinition_t* GetAddonDef() {
    AddonDef.Signature        = 0xCA57A000;
    AddonDef.APIVersion       = NEXUS_API_VERSION;
    AddonDef.Name             = "Cast Away";
    AddonDef.Version.Major    = V_MAJOR;
    AddonDef.Version.Minor    = V_MINOR;
    AddonDef.Version.Build    = V_BUILD;
    AddonDef.Version.Revision = V_REVISION;
    AddonDef.Author           = "PieOrCake.7635";
    AddonDef.Description      =
        "Fishing companion: searchable fish database with time-of-day, bait info, and interactive map. Optional: Hoard & Seek and Events:Alerts.";
    AddonDef.Load             = AddonLoad;
    AddonDef.Unload           = AddonUnload;
    AddonDef.Flags            = AF_None;
    AddonDef.Provider         = UP_GitHub;
    AddonDef.UpdateLink       = "https://github.com/PieOrCake/cast_away";
    return &AddonDef;
}

// ---------------------------------------------------------------------------
// AddonLoad
// ---------------------------------------------------------------------------
void AddonLoad(AddonAPI_t* aApi) {
    APIDefs = aApi;

    ImGui::SetCurrentContext((ImGuiContext*)APIDefs->ImguiContext);
    ImGui::SetAllocatorFunctions(
        (void*(*)(size_t, void*))APIDefs->ImguiMalloc,
        (void(*)(void*, void*))APIDefs->ImguiFree);

    g_NexusLink = (NexusLinkData_t*)APIDefs->DataLink_Get(DL_NEXUS_LINK);

    CastAway::IconManager::Initialize(APIDefs);
    g_AchTracker.Init(APIDefs);

    std::string dataDir = std::string(APIDefs->Paths_GetAddonDirectory("CastAway"));
    std::filesystem::create_directories(dataDir);
    g_MapPanel.Init(APIDefs, dataDir);

    g_SortedFishIndices.resize((size_t)FISH_COUNT);
    std::iota(g_SortedFishIndices.begin(), g_SortedFishIndices.end(), 0);

    APIDefs->GUI_Register(RT_Render, AddonRender);
    APIDefs->GUI_Register(RT_OptionsRender, AddonOptions);

    APIDefs->InputBinds_RegisterWithString(
        "KB_CAST_AWAY_TOGGLE",
        [](const char*, bool rel) { if (!rel) g_WindowVisible = !g_WindowVisible; },
        "CTRL+SHIFT+F");

    APIDefs->GUI_RegisterCloseOnEscape("Cast Away##MainWindow", &g_WindowVisible);

    APIDefs->Textures_LoadFromMemory(TEX_ICON,       (void*)ICON_NORMAL, ICON_NORMAL_size, nullptr);
    APIDefs->Textures_LoadFromMemory(TEX_ICON_HOVER, (void*)ICON_HOVER,  ICON_HOVER_size,  nullptr);

    if (g_ShowQAIcon)
        APIDefs->QuickAccess_Add(QA_ID, TEX_ICON, TEX_ICON_HOVER,
                                 "KB_CAST_AWAY_TOGGLE", "Cast Away");

    LoadSettings();
}

// ---------------------------------------------------------------------------
// AddonUnload
// ---------------------------------------------------------------------------
void AddonUnload() {
    SaveSettings();

    APIDefs->QuickAccess_Remove(QA_ID);
    APIDefs->GUI_DeregisterCloseOnEscape("Cast Away##MainWindow");
    APIDefs->InputBinds_Deregister("KB_CAST_AWAY_TOGGLE");
    APIDefs->GUI_Deregister(AddonOptions);
    APIDefs->GUI_Deregister(AddonRender);

    g_AchTracker.Shutdown();
    g_MapPanel.Shutdown();
    CastAway::IconManager::Shutdown();

    APIDefs = nullptr;
}
