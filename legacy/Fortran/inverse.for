      program geod
      implicit none

      interface
        subroutine direct(a, f, lat1, lon1, azi1, s12a12, arcmod,
     +      lat2, lon2, azi2, omask, a12s12, m12, MM12, MM21, SS12)
        double precision, intent(in) :: a, f, lat1, lon1, azi1, s12a12
        logical, intent(in) :: arcmod
        integer, intent(in) :: omask
        double precision, intent(out) :: lat2, lon2, azi2
* optional output (depending on omask)
        double precision, intent(out) :: a12s12, m12, MM12, MM21, SS12
        end subroutine direct

        subroutine invers(a, f, lat1, lon1, lat2, lon2,
     +      s12, azi1, azi2, omask, a12, m12, MM12, MM21, SS12)
        double precision, intent(in) :: a, f, lat1, lon1, lat2, lon2
        integer, intent(in) :: omask
        double precision, intent(out) :: s12, azi1, azi2
* optional output (depending on omask)
        double precision, intent(out) :: a12, m12, MM12, MM21, SS12
        end subroutine invers
      end interface

      double precision a, f, lat1, lon1, azi1, lat2, lon2, azi2, s12,
     +    dummy
      integer omask

* WGS84 values
      a = 6378137d0
      f = 1/298.257223563d0

      omask = 0

 10   continue
      read(*, *, end=90, err=90) lat1, lon1, lat2, lon2
      call invers(a, f, lat1, lon1, lat2, lon2,
     +    s12, azi1, azi2, omask, dummy, dummy, dummy, dummy, dummy)
      print 20, azi1, azi2, s12
 20   format(f20.15, 1x, f20.15, 1x, f19.10)
      go to 10
 90   continue
      stop
      end
