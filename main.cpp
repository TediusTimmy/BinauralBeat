/*
Copyright (c) 2022, Thomas DiModica
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define OLC_PGE_APPLICATION
#include "include/olcPixelGameEngine.h"
#define OLC_PGEX_SOUND
#include "include/olcPGEX_Sound.h"

#include <functional>

static const double A440 = 440.0; // The A above middle C : ISO standard 16

static const int notesPerOctave = 7;
static const int octavesImplemented = 6;
static const int totalNotes = notesPerOctave * octavesImplemented + 1;
static const int maxBeat = 14;

std::vector<double> generateTwelveToneEqual (double AAboveMiddleC)
 {
   std::vector<double> notes (totalNotes, 0.0);
   double A = AAboveMiddleC / 16.0;
   for (int O = 0; O < octavesImplemented; ++O)
    {
      notes[O * notesPerOctave + 1] = A / std::pow(2, 9.0 / 12.0);
      notes[O * notesPerOctave + 2] = A / std::pow(2, 7.0 / 12.0);
      notes[O * notesPerOctave + 3] = A / std::pow(2, 5.0 / 12.0);
      notes[O * notesPerOctave + 4] = A / std::pow(2, 4.0 / 12.0);
      notes[O * notesPerOctave + 5] = A / std::pow(2, 2.0 / 12.0);
      notes[O * notesPerOctave + 6] = A;
      notes[O * notesPerOctave + 7] = A * std::pow(2, 2.0 / 12.0);
      A *= 2.0;
    }
   return notes;
 }

const std::vector<double>& getStandardTwelveToneEqualNotes()
 {
   static std::vector<double> notes = generateTwelveToneEqual(A440);
   return notes;
 }

std::vector<double> generatePythagorean (double AAboveMiddleC)
 {
   std::vector<double> notes (totalNotes, 0.0);
   double A = AAboveMiddleC / 16.0;
   for (int O = 0; O < octavesImplemented; ++O)
    {
      notes[O * notesPerOctave + 1] = A * (16.0 / 27.0);
      notes[O * notesPerOctave + 2] = A * (2.0 / 3.0);
      notes[O * notesPerOctave + 3] = A * (18.0 / 24.0);
      notes[O * notesPerOctave + 4] = A * (64.0 / 81.0);
      notes[O * notesPerOctave + 5] = A * (8.0 / 9.0);
      notes[O * notesPerOctave + 6] = A;
      notes[O * notesPerOctave + 7] = A * (9.0 / 8.0);
      A *= 2.0;
    }
   return notes;
 }

const std::vector<double>& getStandardPythagoreanNotes()
 {
   static std::vector<double> notes = generatePythagorean(A440);
   return notes;
 }

std::vector<std::string> generateNoteNames()
 {
   std::vector<std::string> result;
   result.emplace_back("R");
   for (int i = 0; i < octavesImplemented; ++i)
    {
      result.emplace_back("C" + std::to_string(i));
      result.emplace_back("D" + std::to_string(i));
      result.emplace_back("E" + std::to_string(i));
      result.emplace_back("F" + std::to_string(i));
      result.emplace_back("G" + std::to_string(i));
      result.emplace_back("A" + std::to_string(i));
      result.emplace_back("B" + std::to_string(i));
    }
   return result;
 }

const std::vector<std::string>& getNoteNames()
 {
   static std::vector<std::string> names = generateNoteNames();
   return names;
 }

static const double PI    = 3.141592653589793238462643383279502884L;
static const double PI_2  = PI * 0.5;
static const double TWOPI = PI * 2.0;

double sineWave (double frequency, double time)
 {
   return std::sin(frequency * TWOPI * time);
 }

double triangularWave (double frequency, double time)
 {
   return std::asin(std::sin(frequency * TWOPI * time)) / PI_2;
 }

double squareWave (double frequency, double time)
 {
   return std::copysign(1.0, std::sin(frequency * TWOPI * time));
 }

double sawWave (double frequency, double time)
 {
   double Time = frequency * time;
   return 2.0 * (Time - std::floor(Time + .5));
 }

double noise (double frequency, double time)
 {
   // The same note played at the same time should produce the same noise.
   return 1.0 - 2.0 * (std::hash<double>{}(frequency * TWOPI * time) / static_cast<double>((std::numeric_limits<size_t>::max)()));
 }

static const int blend = 1000;
static const int steps = 10;

class BinauralBeat : public olc::PixelGameEngine
 {
private:
   int lnote, newLnote;
   int lbias, newLbias;
   int rnote, newRnote;
   int rbias, newRbias;
   int lcounter, rcounter;
   bool lchange, rchange;
   double lastTime;
   bool temperment;
   double hasNoise;

   std::function<double(double, double)> instrument;

   double MyCustomSynthFunction(int nChannel, double fGlobalTime, double /*fTimeStep*/)
    {
      double result = 0;
      lastTime = fGlobalTime;

      if (0 == nChannel)
       {
         if (false == lchange)
          {
            double freq = getNotes()[lnote] + lbias / static_cast<double>(steps);
            result = instrument(freq, fGlobalTime);
          }
         else
          {
            double freq1 = getNotes()[lnote] + lbias / static_cast<double>(steps);
            double freq2 = getNotes()[newLnote] + newLbias / static_cast<double>(steps);
            result = instrument(freq1, fGlobalTime) * (blend - lcounter) / static_cast<double>(blend) + instrument(freq2, fGlobalTime) * lcounter / static_cast<double>(blend);

            ++lcounter;
            if (blend == lcounter)
             {
               lnote = newLnote;
               lbias = newLbias;
               lchange = false;
             }
          }
       }
      else if (1 == nChannel)
       {
         if (false == rchange)
          {
            double freq = getNotes()[rnote] + rbias / static_cast<double>(steps);
            result = instrument(freq, fGlobalTime);
          }
         else
          {
            double freq1 = getNotes()[rnote] + rbias / static_cast<double>(steps);
            double freq2 = getNotes()[newRnote] + newRbias / static_cast<double>(steps);
            result = instrument(freq1, fGlobalTime) * (blend - rcounter) / static_cast<double>(blend) + instrument(freq2, fGlobalTime) * rcounter / static_cast<double>(blend);

            ++rcounter;
            if (blend == rcounter)
             {
               rnote = newRnote;
               rbias = newRbias;
               rchange = false;
             }
          }
       }

      return result * 0.125 + hasNoise * noise(getNotes()[lnote], fGlobalTime) / 64.0;
    }

public:
   BinauralBeat()
    {
      sAppName = "Binaural Beat Explorer";
      lnote = 0;
      newLnote = 4 * 7 + 5 + 1;
      rnote = 0;
      newRnote = 4 * 7 + 5 + 1;
      lbias = 0;
      newLbias = 0;
      rbias = 0;
      newRbias = 0;
      lcounter = 0;
      rcounter = 0;
      lchange = true;
      rchange = true;

      instrument = sineWave;
      lastTime = 0.0;

      temperment = false;
      hasNoise = 0.0;
    }

   const std::vector<double>& getNotes()
    {
      if (false == temperment)
         return getStandardTwelveToneEqualNotes();
      else
         return getStandardPythagoreanNotes();
    }

   bool OnUserCreate() override
    {
      olc::SOUND::InitialiseAudio(48000, 2, 8, 512);
      olc::SOUND::SetUserSynthFunction(std::bind(&BinauralBeat::MyCustomSynthFunction, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
      return true;
    }

   bool OnUserUpdate(float /*fElapsedTime*/) override
    {
      Clear(olc::BLUE);
      DrawString(20, 20, "Base: " + std::to_string(getNotes()[lnote]) + " " + getNoteNames()[lnote]);
      DrawString(20, 40, "Left Ear: " + std::to_string(getNotes()[lnote] + lbias / static_cast<double>(steps)));
      DrawString(20, 60, "Right Ear: " + std::to_string(getNotes()[lnote] + rbias / static_cast<double>(steps)));
      double beat = std::abs(lbias - rbias) / static_cast<double>(steps);
      DrawString(20, 80, "Beat: " + std::to_string(beat));
      if ((beat >= 0.4) && (beat <= 4))
       {
         DrawString(20, 100, "Delta (Sleep)");
       }
      else if ((beat > 4) && (beat <= 8))
       {
         DrawString(20, 100, "Theta (Meditate)");
       }
      else if ((beat > 8) && (beat <= 14))
       {
         DrawString(20, 100, "Alpha (Relax)");
       }
      else if (beat > 14)
       {
         DrawString(20, 100, "Beta (Anxiety)");
       }
      DrawString(20, 120, "Time: " + std::to_string(lastTime));

      if (GetKey(olc::Key::UP).bPressed)
       {
         if ((false == lchange) && (false == rchange))
          {
            if (lnote < (totalNotes - 1))
             {
               newLnote = lnote + 1;
               newRnote = rnote + 1;
               lcounter = 0;
               rcounter = 0;
               lchange = true;
               rchange = true;
             }
          }
       }
      if (GetKey(olc::Key::DOWN).bPressed)
       {
         if ((false == lchange) && (false == rchange))
          {
            if (lnote > 0)
             {
               newLnote = lnote - 1;
               newRnote = rnote - 1;
               lcounter = 0;
               rcounter = 0;
               lchange = true;
               rchange = true;
             }
          }
       }
      if (GetKey(olc::Key::W).bPressed)
       {
         if ((false == lchange) && (false == rchange))
          {
            if (std::abs((lbias + 1) - rbias) <= (maxBeat * steps))
             {
               newLnote = lnote;
               newLbias = lbias + 1;
               lcounter = 0;
               lchange = true;
             }
          }
       }
      if (GetKey(olc::Key::S).bPressed)
       {
         if ((false == lchange) && (false == rchange))
          {
            if (std::abs((lbias - 1) - rbias) <= (maxBeat * steps))
             {
               newLnote = lnote;
               newLbias = lbias - 1;
               lcounter = 0;
               lchange = true;
             }
          }
       }
      if (GetKey(olc::Key::A).bPressed)
       {
         if ((false == lchange) && (false == rchange))
          {
            if (std::abs(lbias - (rbias + 1)) <= (maxBeat * steps))
             {
               newRnote = rnote;
               newRbias = rbias + 1;
               rcounter = 0;
               rchange = true;
             }
          }
       }
      if (GetKey(olc::Key::D).bPressed)
       {
         if ((false == lchange) && (false == rchange))
          {
            if (std::abs(lbias - (rbias - 1)) <= (maxBeat * steps))
             {
               newRnote = rnote;
               newRbias = rbias - 1;
               rcounter = 0;
               rchange = true;
             }
          }
       }
      if (GetKey(olc::Key::Z).bPressed)
       {
         instrument = sineWave;
       }
      if (GetKey(olc::Key::X).bPressed)
       {
         instrument = triangularWave;
       }
      if (GetKey(olc::Key::C).bPressed)
       {
         instrument = squareWave;
       }
      if (GetKey(olc::Key::V).bPressed)
       {
         instrument = sawWave;
       }
      if (GetKey(olc::Key::B).bPressed)
       {
         temperment = !temperment;
       }
      if (GetKey(olc::Key::N).bPressed)
       {
         hasNoise = 1.0 - hasNoise;
       }
      if (GetKey(olc::Key::Q).bHeld)
       {
         return false;
       }
      return true;
    }

   bool OnUserDestroy()
    {
      olc::SOUND::DestroyAudio();
      return true;
    }
 };

int main (int /*argc*/, char ** /*argv*/)
 {
   BinauralBeat demo;
   if (demo.Construct(320, 240, 2, 2))
    {
      demo.Start();
    }
   return 0;
 }
