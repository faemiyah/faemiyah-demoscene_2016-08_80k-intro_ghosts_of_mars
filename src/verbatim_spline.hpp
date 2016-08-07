#ifndef VERBATIM_SPLINE_HPP
#define VERBATIM_SPLINE_HPP

#include "verbatim_spline_point.hpp"

/// Spline mode.
enum SplineMode
{
  WEIGHTED = 0,
  BEZIER = 1
};

/// Spline interpolator.
class Spline
{
  private:
    /// Sequence of points.
    seq<SplinePoint> m_points;

    /// Interpolation mode (false == trilinear, true == bezier).
    bool m_mode;

  public:
    /// Constructor.
    ///
    /// \param mode Spline mode.
    Spline(SplineMode mode) :
      m_mode(mode) { }

  private:
    /// Add spline point.
    ///
    /// \param pos Position.
    /// \param stamp Timestamp.
    void addPoint(const vec3 &pos, float stamp)
    {
      m_points.emplace_back(pos, stamp);
    }
    /// Add spline point wrapper
    ///
    /// \param op1 Operand 1 (X coordinate).
    /// \param op2 Operand 2 (Y coordinate).
    /// \param op3 Operand 3 (Z coordinate).
    /// \param op4 Operand 4 (timestamp).
    void addPoint(int16_t op1, int16_t op2, int16_t op3, int16_t op4)
    {
#if defined(USE_LD)
      if(0 > op4)
      {
        std::ostringstream sstr;
        sstr << "tried to add spline point with invalid timestamp: " << op4;
        BOOST_THROW_EXCEPTION(std::runtime_error(sstr.str()));
      }
#endif
      addPoint(vec3(static_cast<float>(op1), static_cast<float>(op2), static_cast<float>(op3)),
            static_cast<float>(op4));
    }

    /// Spline interpolation (bezier).
    ///
    /// \param idx Base index.
    /// \param interp Interpolation value.
    vec3 splineInterpolateBezier(unsigned idx, float interp) const
    {
      int sidx = static_cast<int>(idx);
      const SplinePoint &curr = getPointClamped(sidx);
      const SplinePoint &next = getPointClamped(sidx + 1);
      const vec3& aa = curr.getPoint();
      const vec3& bb = curr.getNext();
      const vec3& cc = next.getPrev();
      const vec3& dd = next.getPoint();
      vec3 ee = mix(aa, bb, interp);
      vec3 ff = mix(bb, cc, interp);
      vec3 gg = mix(cc, dd, interp);
      vec3 hh =  mix(ee, ff, interp);
      vec3 ii =  mix(ff, gg, interp);

      return mix(hh, ii, interp);
    }

    /// Spline interpolation (4-point weighted).
    ///
    /// \param idx Base index.
    /// \param interp Interpolation value.
    vec3 splineInterpolateWeighted(unsigned idx, float interp) const
    {
      int sidx = static_cast<int>(idx);
      const vec3 &aa = getPointClamped(sidx - 1).getPoint();
      const vec3 &bb = getPointClamped(sidx + 0).getPoint();
      const vec3 &cc = getPointClamped(sidx + 1).getPoint();
      const vec3 &dd = getPointClamped(sidx + 2).getPoint();
      vec3 ee = mix(aa, bb, interp);
      vec3 ff = mix(bb, cc, interp);
      vec3 gg = mix(cc, dd, interp);

      return (ee + ff + gg) * (1.0f / 3.0f);
    }

    /// Return point position at given point, or first/last points if outside spline.
    ///
    /// \param idx Index to access.
    /// \return Point at index.
    const SplinePoint& getPointClamped(int idx) const
    {
      if(static_cast<int>(m_points.size()) <= idx)
      {
        return m_points.back();
      }
      if(0 > idx)
      {
        return m_points.front();
      }
      return m_points[static_cast<unsigned>(idx)];
    }

    /// Precalculate points for spline.
    void precalculate()
    {
      for(int ii = 0; (static_cast<int>(m_points.size()) > ii); ++ii)
      {
        SplinePoint &vv = m_points[ii];
        const vec3& prev = getPointClamped(ii - 1).getPoint();
        const vec3& next = getPointClamped(ii + 1).getPoint();
        const vec3& curr = vv.getPoint();

        vv.setPrev(normalize(prev - next) * dnload_sqrtf(length(prev - curr)) + curr);
        vv.setNext(normalize(next - prev) * dnload_sqrtf(length(next - curr)) + curr);
      }
    }

  public:
    /// Read data.
    ///
    /// \param data Data iteration pointer.
    /// \return Data pointer after read.
    const int16_t* readData(const int16_t *data)
    {
      const int16_t *iter = data;
      for(;;)
      {
        const int16_t *next = iter + 4;
        if(is_segment_end(iter))
        {
          precalculate();
          return next;
        }
        addPoint(iter[0], iter[1], iter[2], iter[3]);
        iter = next;
      }
    }
    
    /// Get position at given time.
    ///
    /// \param stamp Timestamp to get position for.
    vec3 resolvePosition(float stamp) const
    {
#if defined(USE_LD)
      if(m_points.empty())
      {
        BOOST_THROW_EXCEPTION(std::runtime_error("incorrect spline data"));
      }
#endif

      float current_time = 0.0f;
      for(unsigned ii = 0; (m_points.size() > ii); ++ii)
      {
        const SplinePoint &vv = m_points[ii];
        float current_segment = vv.getTimestamp();

        if((current_time + current_segment) > stamp)
        {
          float interp = (stamp - current_time) / current_segment;

          if(m_mode == BEZIER)
          {
            return splineInterpolateBezier(ii, interp);
          }
          return splineInterpolateWeighted(ii, interp);
        }
        current_time += current_segment;
      }

      return m_points.back().getPoint();
    }
    /// Resolve position wrapper.
    ///
    /// \param stamp Timestamp to get position for.
    vec3 resolvePosition(unsigned stamp) const
    {
      return resolvePosition(static_cast<float>(stamp));
    }

  public:
    /// Tell if this is an end of a spline segment data blob.
    ///
    /// \param data Input data.
    /// \return True if yes, false if no.
    static bool is_segment_end(const int16_t *data)
    {
      return ((0 == data[0]) &&
          (0 == data[1]) &&
          (0 == data[2]) &&
          (0 == data[3]));
    }
};

#endif
