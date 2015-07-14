/*
 * Copyright (C) 2014 Bernt Arne Oedegaard
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * revised by wb, xiaoyem
 */

#ifndef FD_INCLUDED
#define FD_INCLUDED

/* FIXME: exported functions */
extern double fd_euro_call(double spot, double strike, double r, double d, double vol,
		double expiry, int ssteps, int tsteps);
extern double fd_euro_put (double spot, double strike, double r, double d, double vol,
		double expiry, int ssteps, int tsteps);
extern double fd_amer_call(double spot, double strike, double r, double d, double vol,
		double expiry, int ssteps, int tsteps);
extern double fd_amer_put (double spot, double strike, double r, double d, double vol,
		double expiry, int ssteps, int tsteps);

#endif /* FD_INCLUDED */

