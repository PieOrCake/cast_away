// Animated fishtank background — ported from Tyrian IM's Fishtank theme.
#include "FishTank.h"
#include <cmath>

static auto hashF = [](int a, int b) -> float {
    unsigned int hv = (unsigned int)(a * 1973 + b * 9781 + 31);
    hv ^= (hv >> 16); hv *= 0x45d9f3bu; hv ^= (hv >> 16);
    return (float)(hv & 0xFFFFu) / 65536.0f;
};

static void Bubbles(ImDrawList* dl, ImVec2 mn, ImVec2 mx, int count) {
    float w = mx.x - mn.x, h = mx.y - mn.y;
    float t = (float)ImGui::GetTime();
    for (int i = 0; i < count; i++) {
        float phi    = i * 0.6180339f;
        float speed  = 14.0f + phi * 22.0f;
        float wobble = sinf(t * 0.5f + phi * 6.0f) * 10.0f;
        float px     = mn.x + fmodf(phi * w * 1.7f + wobble, w);
        float py     = mx.y - fmodf(t * speed + phi * h, h);
        float pulse  = 0.4f + 0.6f * sinf(t * 1.4f + phi * 5.0f);
        float r      = 1.0f + pulse * 2.0f;
        int   a      = (int)(8 + pulse * 22);
        ImU32 col    = (i % 2 == 0) ? IM_COL32(180,230,255,a) : IM_COL32(220,245,255,a);
        dl->AddCircleFilled(ImVec2(px, py), r, col);
        if (i % 5 == 0 && pulse > 0.7f)
            dl->AddCircleFilled(ImVec2(px, py), r * 2.5f, IM_COL32(180,230,255,(int)(a*0.25f)));
    }
}

static void Seaweed(ImDrawList* dl, ImVec2 mn, ImVec2 mx) {
    float t = (float)ImGui::GetTime();
    float w = mx.x - mn.x;
    for (int i = 0; i < 14; i++) {
        float baseX   = mn.x + (0.03f + hashF(i,1) * 0.94f) * w;
        float stalkH  = 60.0f + hashF(i,2) * 100.0f;
        float phase   = hashF(i,3) * 6.28318f;
        float swayAmt = 10.0f + hashF(i,4) * 14.0f;
        float sway    = sinf(t * 0.6f + phase) * swayAmt;
        float sway2   = sinf(t * 0.6f + phase + 0.8f) * swayAmt * 0.6f;
        ImVec2 p0(baseX,          mx.y);
        ImVec2 p1(baseX + sway2,  mx.y - stalkH * 0.35f);
        ImVec2 p2(baseX + sway,   mx.y - stalkH * 0.70f);
        ImVec2 p3(baseX + sway,   mx.y - stalkH);
        dl->AddBezierCubic(p0, p1, p2, p3, IM_COL32(20,110,55,200), 2.5f);
        dl->AddBezierCubic(p0, p1, p2, p3, IM_COL32(45,180,85, 80), 1.0f);
        float leafCx = p3.x + sinf(t * 0.5f + phase) * 3.0f;
        float leafCy = p3.y - 4.0f;
        dl->PathClear();
        for (int k = 0; k <= 8; k++) {
            float a = (float)k / 8.0f * 6.28318f;
            dl->PathLineTo(ImVec2(leafCx + cosf(a) * 4.5f, leafCy + sinf(a) * 8.0f));
        }
        dl->PathFillConvex(IM_COL32(30,160,70,150));
    }
}

void DrawFishtankBg(ImDrawList* dl, ImVec2 mn, ImVec2 mx) {
    float t = (float)ImGui::GetTime();
    float w = mx.x - mn.x, h = mx.y - mn.y;

    // Deep ocean gradient
    dl->AddRectFilledMultiColor(mn, mx,
        IM_COL32(4,18,50,255), IM_COL32(4,18,50,255),
        IM_COL32(2, 8,25,255), IM_COL32(2, 8,25,255));

    // Sandy bottom
    dl->AddRectFilledMultiColor(
        ImVec2(mn.x, mx.y-18.f), mx,
        IM_COL32(160,135,70,  0), IM_COL32(160,135,70,  0),
        IM_COL32(160,135,70,180), IM_COL32(160,135,70,180));

    // Caustic shimmer
    const float shFreq[2]  = {2.4f, 1.7f};
    const float shSpeed[2] = {0.18f, 0.25f};
    const ImU32 shCol[2]   = {IM_COL32(100,200,255,8), IM_COL32(200,240,255,6)};
    float bandH = h * 0.12f;
    for (int layer = 0; layer < 2; layer++) {
        float segW = w / 16.f;
        for (int s = 0; s < 16; s++) {
            float x0  = mn.x + s*segW, x1 = x0 + segW;
            float xn0 = (float)s/16.f,  xn1 = (float)(s+1)/16.f;
            float y0  = mn.y + bandH*(0.3f + 0.7f*sinf(shFreq[layer]*xn0*6.28318f + t*shSpeed[layer]));
            float y1  = mn.y + bandH*(0.3f + 0.7f*sinf(shFreq[layer]*xn1*6.28318f + t*shSpeed[layer]));
            dl->AddTriangleFilled(ImVec2(x0,mn.y), ImVec2(x1,mn.y), ImVec2(x0,y0), shCol[layer]);
            dl->AddTriangleFilled(ImVec2(x1,mn.y), ImVec2(x1,y1),  ImVec2(x0,y0), shCol[layer]);
        }
    }

    // Suspended particles
    for (int i = 0; i < 70; i++) {
        float px    = mn.x + hashF(i,20)*w + sinf(t*0.15f + hashF(i,22)*6.28318f)*4.f;
        float py    = mn.y + hashF(i,21)*h + sinf(t*0.10f + hashF(i,22)*6.28318f + 1.1f)*3.f;
        float r     = 0.8f + hashF(i,23)*1.2f;
        float pulse = 0.5f + 0.5f*sinf(t*0.3f + hashF(i,22)*12.566f);
        int   a     = (int)(5 + pulse*13);
        dl->AddCircleFilled(ImVec2(px,py), r, IM_COL32(140,210,255,a), 4);
    }

    Bubbles(dl, mn, mx, 30);
    Seaweed(dl, mn, mx);
}

void DrawFishtankCreatures(ImDrawList* dl, ImVec2 mn, ImVec2 mx) {
    float t = (float)ImGui::GetTime();
    float w = mx.x - mn.x, h = mx.y - mn.y;

    // Swimming fish
    const ImU32 fishCol[5] = {
        IM_COL32(255,120, 40,210), IM_COL32( 80,200,240,210),
        IM_COL32(240,220, 50,210), IM_COL32(200, 60,180,210),
        IM_COL32( 90,220,130,210),
    };
    for (int i = 0; i < 5; i++) {
        float speed     = 18.0f + hashF(i,1) * 28.0f;
        float homeY     = mn.y + (0.06f + fmodf((float)i * 0.6180339f, 1.f) * 0.78f) * h;
        float bodyRX    = 6.0f  + hashF(i,3) * 18.0f;
        float bodyRY    = 4.0f  + hashF(i,4) * 10.0f;
        float vertFreq  = 0.5f  + hashF(i,6) * 1.0f;
        float vertAmp   = 6.0f  + hashF(i,7) * 10.0f;
        float vertPhase = hashF(i,8) * 6.28318f;

        float cycle       = fmodf(t * speed + hashF(i,5) * 2.f * w, 2.f * w);
        bool  facingRight = cycle < w;
        float px = facingRight ? (mn.x + cycle) : (mx.x - (cycle - w));
        px = fmaxf(mn.x + bodyRX + 4.f, fminf(mx.x - bodyRX - 4.f, px));
        float py = homeY + sinf(t * vertFreq + vertPhase) * vertAmp;
        py = fmaxf(mn.y + bodyRY + 8.f, fminf(mx.y - 55.f - bodyRY, py));

        dl->PathClear();
        for (int k = 0; k <= 12; k++) {
            float a = (float)k / 12.f * 6.28318f;
            dl->PathLineTo(ImVec2(px + cosf(a)*bodyRX, py + sinf(a)*bodyRY));
        }
        dl->PathFillConvex(fishCol[i]);

        float tailDir = facingRight ? -1.f : 1.f;
        ImVec2 ta(px + tailDir*bodyRX,                   py);
        ImVec2 tb(px + tailDir*(bodyRX + bodyRY*1.4f),   py - bodyRY*0.7f);
        ImVec2 tc(px + tailDir*(bodyRX + bodyRY*1.4f),   py + bodyRY*0.7f);
        dl->AddTriangleFilled(ta, tb, tc, fishCol[i]);

        float eyeX = px + (facingRight ? 1.f : -1.f) * bodyRX * 0.55f;
        float eyeY = py - bodyRY * 0.3f;
        dl->AddCircleFilled(ImVec2(eyeX, eyeY), 1.8f, IM_COL32(255,255,255,220), 6);
        dl->AddCircleFilled(ImVec2(eyeX, eyeY), 0.7f, IM_COL32(  0,  0,  0,220), 4);
    }

    // School of three fish swimming together in a V formation
    {
        const float speed = 22.f;
        float cycle = fmodf(t * speed + 137.f, 2.f * w);
        bool ltr = cycle < w;
        float baseX = ltr ? (mn.x + cycle) : (mx.x - (cycle - w));
        float baseY = mn.y + 0.42f * h + sinf(t * 0.6f) * 14.f;
        float dir = ltr ? 1.f : -1.f;
        const ImU32 schoolCol = IM_COL32(170, 215, 255, 220);
        const float bodyRX = 6.5f, bodyRY = 3.8f;
        // Leader at index 0, two wingmen offset behind/above and behind/below
        const float offX[3] = { 0.f,  18.f, 18.f };
        const float offY[3] = { 0.f,  -8.f,  8.f };

        for (int i = 0; i < 3; i++) {
            float px = baseX - dir * offX[i];
            float py = baseY + offY[i] + sinf(t * 1.3f + (float)i * 1.7f) * 1.6f;
            if (px < mn.x - 30.f || px > mx.x + 30.f) continue;

            dl->PathClear();
            for (int k = 0; k <= 12; k++) {
                float a = (float)k / 12.f * 6.28318f;
                dl->PathLineTo(ImVec2(px + cosf(a)*bodyRX, py + sinf(a)*bodyRY));
            }
            dl->PathFillConvex(schoolCol);
            float tailDir = -dir;
            ImVec2 ta(px + tailDir*bodyRX,                 py);
            ImVec2 tb(px + tailDir*(bodyRX + bodyRY*1.4f), py - bodyRY*0.7f);
            ImVec2 tc(px + tailDir*(bodyRX + bodyRY*1.4f), py + bodyRY*0.7f);
            dl->AddTriangleFilled(ta, tb, tc, schoolCol);
            float eyeX = px + dir * bodyRX * 0.55f;
            float eyeY = py - bodyRY * 0.3f;
            dl->AddCircleFilled(ImVec2(eyeX, eyeY), 1.4f, IM_COL32(255,255,255,220), 6);
            dl->AddCircleFilled(ImVec2(eyeX, eyeY), 0.6f, IM_COL32(0,0,0,220), 4);
        }
    }

    // A second, larger school of six fish in a loose flock formation
    {
        const float speed = 16.f;
        float cycle = fmodf(t * speed + 421.f, 2.f * w);
        bool ltr = cycle < w;
        float baseX = ltr ? (mn.x + cycle) : (mx.x - (cycle - w));
        float baseY = mn.y + 0.68f * h + sinf(t * 0.45f) * 18.f;
        float dir = ltr ? 1.f : -1.f;
        const ImU32 schoolCol = IM_COL32(255, 200, 150, 220);
        const float bodyRX = 7.f, bodyRY = 4.2f;
        // Two rows of three in a diamond cluster behind the leader
        const float offX[6] = { 0.f,  16.f, 16.f, 30.f, 30.f, 30.f };
        const float offY[6] = { 0.f, -10.f, 10.f,-18.f,  0.f, 18.f };

        for (int i = 0; i < 6; i++) {
            float px = baseX - dir * offX[i];
            float py = baseY + offY[i] + sinf(t * 1.5f + (float)i * 1.3f) * 1.8f;
            if (px < mn.x - 30.f || px > mx.x + 30.f) continue;

            dl->PathClear();
            for (int k = 0; k <= 12; k++) {
                float a = (float)k / 12.f * 6.28318f;
                dl->PathLineTo(ImVec2(px + cosf(a)*bodyRX, py + sinf(a)*bodyRY));
            }
            dl->PathFillConvex(schoolCol);
            float tailDir = -dir;
            ImVec2 ta(px + tailDir*bodyRX,                 py);
            ImVec2 tb(px + tailDir*(bodyRX + bodyRY*1.4f), py - bodyRY*0.7f);
            ImVec2 tc(px + tailDir*(bodyRX + bodyRY*1.4f), py + bodyRY*0.7f);
            dl->AddTriangleFilled(ta, tb, tc, schoolCol);
            float eyeX = px + dir * bodyRX * 0.55f;
            float eyeY = py - bodyRY * 0.3f;
            dl->AddCircleFilled(ImVec2(eyeX, eyeY), 1.4f, IM_COL32(255,255,255,220), 6);
            dl->AddCircleFilled(ImVec2(eyeX, eyeY), 0.6f, IM_COL32(0,0,0,220), 4);
        }
    }

    // Rare easter-egg shark cruising mid-water across the tank.
    const float SHARK_DUR = 7.f;
    float epochT = 0.f;
    for (int j = 0; j < 512; j++) {
        float gap      = 30.f + hashF(j,17) * 30.f;
        float cycleLen = gap + SHARK_DUR;
        if (t < epochT + cycleLen) {
            float localT = t - epochT;
            if (localT >= gap) {
                float progress = (localT - gap) / SHARK_DUR;
                bool  ltr      = hashF(j,7) > 0.5f;
                float dir      = ltr ? 1.f : -1.f;
                float cx       = ltr ? (mn.x - 120.f + progress*(w+240.f))
                                     : (mx.x + 120.f - progress*(w+240.f));
                float cy       = mn.y + 0.50f * h + sinf(t * 0.35f + (float)j) * 16.f;

                // Twice the previous size (was bRX=36, bRY=10).
                const float bRX = 72.f, bRY = 20.f;
                ImU32 top   = IM_COL32( 95, 110, 130, 235);
                ImU32 belly = IM_COL32(180, 190, 205, 225);

                // Belly (lighter, shifted down a touch)
                dl->PathClear();
                for (int k = 0; k <= 16; k++) {
                    float a = (float)k / 16.f * 6.28318f;
                    dl->PathLineTo(ImVec2(cx + cosf(a)*bRX, cy + 4.f + sinf(a)*bRY));
                }
                dl->PathFillConvex(belly);
                // Top body
                dl->PathClear();
                for (int k = 0; k <= 16; k++) {
                    float a = (float)k / 16.f * 6.28318f;
                    dl->PathLineTo(ImVec2(cx + cosf(a)*bRX, cy + sinf(a)*bRY));
                }
                dl->PathFillConvex(top);
                // Dorsal fin
                ImVec2 df1(cx,            cy - bRY);
                ImVec2 df2(cx - dir*24.f, cy - bRY - 32.f);
                ImVec2 df3(cx - dir*36.f, cy - bRY);
                dl->AddTriangleFilled(df1, df2, df3, top);
                // Tail
                float tailX = cx - dir * (bRX + 4.f);
                ImVec2 t1(tailX,                cy);
                ImVec2 t2(tailX - dir * 32.f,   cy - 28.f);
                ImVec2 t3(tailX - dir * 32.f,   cy + 24.f);
                dl->AddTriangleFilled(t1, t2, t3, top);
                // Pectoral fin
                ImVec2 pf1(cx + dir * 16.f, cy + bRY * 0.6f);
                ImVec2 pf2(cx + dir *  4.f, cy + bRY + 24.f);
                ImVec2 pf3(cx - dir *  8.f, cy + bRY * 0.6f);
                dl->AddTriangleFilled(pf1, pf2, pf3, top);
                // Eye + gills
                float ex = cx + dir * bRX * 0.55f;
                float ey = cy - bRY * 0.2f;
                dl->AddCircleFilled(ImVec2(ex, ey), 3.6f, IM_COL32(0,0,0,235), 8);
                for (int g = 0; g < 3; g++) {
                    float gx = cx + dir * (bRX * 0.30f - (float)g * 10.f);
                    dl->AddLine(ImVec2(gx, cy - 4.f), ImVec2(gx, cy + 7.f),
                                IM_COL32(60, 70, 90, 200), 1.5f);
                }
            }
            break;
        }
        epochT += cycleLen;
    }
}
