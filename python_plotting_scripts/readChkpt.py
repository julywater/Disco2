from itertools import imap
import h5py as h5
import numpy as np

def readChkpt(filename):

    f = h5.File(filename)
    Data = f['Data'][...]
    t = f['T'][0]
    f.close()

    #Standard way of removing duplicates
    Data = np.array(dict(zip(map(hash, map(tuple,Data[:,0:3])), Data)).values())
    #read in coords
    phi = np.array(Data[:,0])
    r = np.array(Data[:,1])
    z = np.array(Data[:,2])

    #read in data
    rho = np.array(Data[:,3])
    P = np.array(Data[:,4])
    vr = np.array(Data[:,5])
    vp = np.array(Data[:,6])
    vz = np.array(Data[:,7])

    #calculate cell volumes
    z_vals = np.unique(z)
    r_vals = np.unique(r)
    dV = np.ones(len(r[(r>r_vals[1])*(r<r_vals[-2])]))
    r_less = r[(r>r_vals[1])*(r<r_vals[-2])]
    phi_less = phi[(r>r_vals[1])*(r<r_vals[-2])]
    z_less = z[(r>r_vals[1])*(r<r_vals[-2])]

    #dz
    if len(z_vals) > 1:
        dV[:] = (z_vals[len(z_vals)-1] - z_vals[0]) / (len(z_vals) - 1.0)

    #dr
    Rp = 0.5*(r_vals[1]+r_vals[2])
    for i in range(2,len(r_vals)-2):
        Rm = Rp
        R = r_vals[i]
        Rp = 2.0*R - Rm
        dV[r_less==R] *= 0.5*(Rp*Rp - Rm*Rm)

    #dphi
    for i in range(2,len(r_vals)-2):
        inds = (r_less==r_vals[i])
        dphi = np.zeros(len(phi[inds]))
        my_phi = np.sort(np.array(phi[inds]))
        dphi[1:] = my_phi[1:] - my_phi[:-1]
        dphi[0] = my_phi[0] - my_phi[-1]
        while (dphi<0).any():
            dphi[dphi<0] += 2*np.pi
        dV[inds] *= dphi

    return t, r, phi, z, rho, P, vr, vp, vz, dV

