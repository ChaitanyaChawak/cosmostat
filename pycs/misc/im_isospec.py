import numpy as np
from os import remove
from subprocess import check_call
from subprocess import call
from datetime import datetime
from astropy.io import fits
import shlex

import pycs as ps
import pycs
from pycs.sparsity.sparse2d.starlet import *
from pycs.misc.cosmostat_init import *
from pycs.misc.mr_prog import *


def im_isospec(ima, mask=None):
    """
    Compute the 1D power spectrum of an image using the c++ program im_isospec

    Parameters
    ----------
    ima : np.ndarray
        input image.

    Returns
    -------
    1D np array
        power spectrum.

    """
    nx, ny = ima.shape
    pad_ke = np.pad(ima, nx // 2)
    p = mr_prog(pad_ke, prog="im_isospec")
    (nx, ny) = p.shape
    p1 = p[1, :].reshape(ny) * 4.0
    # print(p1.shape)
    npix = p1.shape
    npix = npix[0] // 2
    # print(type(p1))
    p2 = rebin1d(p1[0 : 2 * npix], [npix])
    if mask is not None:
        fsky = mask.sum() / mask.size
        p2 = p2 / fsky
    return p2
