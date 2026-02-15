#pragma once

#include "Note.h"
#include <set>
#include <vector>

namespace audio_plugin {
/**
 * @brief Zones divide time into active/inactive parts. They determine the activity of notes for
 * building keys and for intelligent analysis (pitch memory model). Visually, they are displayed on
 * the top panel.
 */
class Zones {
  public:
    /**
     * @brief Construct a Zones object with single initial zone
     * @param initialOnOff Whether the initial zone should be active
     * @param borderPoint The right border point (left border is always 0)
     */
    Zones(bool initialOnOff, float borderPoint) : borderPoint(borderPoint) {
        zonesOnOff.push_back(initialOnOff);
    }

    /**
     * @brief Construct a Zones object with predefined zones
     * @param borderPoint The right border point
     * @param zonesPoints Set of zones boundary points
     * @param zonesOnOff Vector of zone states (true = active, false = inactive)
     */
    Zones(float borderPoint, const std::set<float> &zonesPoints,
          const std::vector<bool> &zonesOnOff)
        : borderPoint(borderPoint), zonesPoints(zonesPoints), zonesOnOff(zonesOnOff) {}

    /**
     * @brief Add a new zone boundary point
     * @param point Time position for the new boundary
     * @return true if point was added, false if point already exists
     */
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

    /**
     * @brief Remove a zone boundary point near the specified position
     * @param point Time position to search near
     * @param precision Maximum distance to consider a point as matching
     * @return true if point was removed, false if no point found
     */
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

    /**
     * @brief Turn the zone at the specified point on
     * @param point Time position within the zone to activate
     */
    void turnZoneOn(float point) {
        auto lower = zonesPoints.lower_bound(point);
        size_t pos = std::distance(zonesPoints.begin(), lower);
        zonesOnOff[pos] = true;
    }

    /**
     * @brief Turn the zone at the specified point off
     * @param point Time position within the zone to deactivate
     */
    void turnZoneOff(float point) {
        auto lower = zonesPoints.lower_bound(point);
        size_t pos = std::distance(zonesPoints.begin(), lower);
        zonesOnOff[pos] = false;
    }

    /**
     * @brief Set the right border point of the timeline
     * @param newBorderPoint New border time position
     * @note Removes any zones that extend beyond the new border
     */
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

    /**
     * @brief Check if a note falls within any active zone
     * @param note The note to check
     * @return true if note overlaps with any active zone, false otherwise
     */
    bool isNoteInActiveZone(const Note &note) {
        if (zonesPoints.empty()) {
            return zonesOnOff[0];
        }
        if (note.time >= borderPoint) {
            return false;
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

    /**
     * @brief Get all zone boundary points (without border points)
     * @return Set of time positions
     */
    const std::set<float> &getZonesPoints() { return zonesPoints; }

    /**
     * @brief Get the on/off state of all zones
     * @return Vector of booleans for each zone from left to right
     */
    const std::vector<bool> &getZonesOnOff() { return zonesOnOff; }

  private:
    ///< borderPoint = right border point (because left border point is always zero)
    float borderPoint;
    std::set<float> zonesPoints;
    std::vector<bool> zonesOnOff;

    /**
     * @brief Get zone index for a given point
     * @param point Time position
     * @param equalMeansRightZone If true, exact match returns right zone; if false, returns left
     * zone
     * @return Zone index
     */
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