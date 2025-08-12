#pragma once

#include "Note.h"
#include <set>
#include <vector>

namespace audio_plugin {
class Zones {
  public:
    Zones(bool initialOnOff, float borderPoint) : borderPoint(borderPoint) {
        zonesOnOff.push_back(initialOnOff);
    }

    Zones(float borderPoint, const std::set<float> &zonesPoints,
          const std::vector<bool> &zonesOnOff)
        : borderPoint(borderPoint), zonesPoints(zonesPoints), zonesOnOff(zonesOnOff) {}

    bool addPoint(float point) {
        auto [it, inserted] = zonesPoints.insert(point);
        if (inserted) {
            size_t pos = std::distance(zonesPoints.begin(), it);
            zonesOnOff.insert(zonesOnOff.begin() + pos, zonesOnOff[pos]);
            return true;
        }
        return false;
    }

    // Doesn't return point 0.0f and borderPoint, because there is no need
    /*std::optional<float> getPoint(float point, float precision) {
        auto lower = zonesPoints.lower_bound(point - precision);
        if (lower != zonesPoints.end() && *lower <= point + precision) {
            return *lower;
        }
        return std::nullopt;
    }*/

    bool removePoint(float point, float precision) {
        auto lower = zonesPoints.lower_bound(point - precision);
        if (lower != zonesPoints.end() && *lower <= point + precision) {
            size_t pos = std::distance(zonesPoints.begin(), lower);
            zonesPoints.erase(lower);
            bool leftZoneOnOff = zonesOnOff[pos];
            bool rightZoneOnOff = zonesOnOff[pos + 1];
            zonesOnOff.erase(zonesOnOff.begin() + pos);
            zonesOnOff[pos] = leftZoneOnOff || rightZoneOnOff;
            return true;
        }
        return false;
    }

    void turnZoneOn(float point) {
        auto lower = zonesPoints.lower_bound(point);
        size_t pos = std::distance(zonesPoints.begin(), lower);
        zonesOnOff[pos] = true;
    }

    void turnZoneOff(float point) {
        auto lower = zonesPoints.lower_bound(point);
        size_t pos = std::distance(zonesPoints.begin(), lower);
        zonesOnOff[pos] = false;
    }

    void setBorderPoint(float newBorderPoint) {
        borderPoint = newBorderPoint;

        auto it = zonesPoints.lower_bound(borderPoint);
        if (it != zonesPoints.end()) {
            size_t numDeletedPoints = std::distance(it, zonesPoints.end());
            zonesPoints.erase(it, zonesPoints.end());
            zonesOnOff.resize(zonesOnOff.size() - numDeletedPoints);
        }
    }

    void turnOnAllZones() {
        for (int i = 0; i < zonesOnOff.size(); ++i)
            zonesOnOff[i] = true;
    }

    void turnOffAllZones() {
        for (int i = 0; i < zonesOnOff.size(); ++i)
            zonesOnOff[i] = false;
    }

    bool isNoteInActiveZone(const Note &note) {
        if (zonesPoints.empty()) {
            return zonesOnOff[0];
        }
        int noteStartZoneInd = getZoneIndex(note.time, false);
        int noteEndZoneInd = getZoneIndex(note.time + note.duration, true);
        for (int zoneInd = noteStartZoneInd; zoneInd <= noteEndZoneInd; ++zoneInd) {
            if (zonesOnOff[zoneInd]) {
                return true;
            }
        }
        return false;
    }

    const std::set<float> &getZonesPoints() { return zonesPoints; }

    const std::vector<bool> &getZonesOnOff() { return zonesOnOff; }

  private:
    // borderPoint = right border point, because left border point is always zero
    float borderPoint;
    std::set<float> zonesPoints;
    std::vector<bool> zonesOnOff;

    int getZoneIndex(float point, bool equalMeansRightZone) {
        std::set<float>::iterator it;
        if (equalMeansRightZone) {
            it = zonesPoints.lower_bound(point);
        } else {
            it = zonesPoints.upper_bound(point);
        }
        size_t pos = std::distance(zonesPoints.begin(), it);
        return int(pos);
    }
};
} // namespace audio_plugin