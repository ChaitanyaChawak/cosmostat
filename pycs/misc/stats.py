# -*- coding: utf-8 -*-

"""STATS MODULE

This module contains some common statistical measures useful in weak-lensing
studies. For example, the higher-order moments of filtered convergence maps
can be used to constrain cosmological parameters.

"""

import numpy as np
from scipy.optimize import brentq
from scipy.stats import norm, gaussian_kde
import scipy
from modopt.math.stats import *


################################################


def get_noise(data):
    m = data - scipy.signal.medfilt(data)
    return mad(m) / 0.6747


################################################


def skew(x):
    """Compute the skewness of an array as the third standardized moment.

    Parameters
    ----------
    x : array_like
        Input array. If `x` is multidimensional, skewness is automatically
        computed over all elements in the array, not just along one axis.

    Returns
    -------
    float
        Skewness of `x`.

    Notes
    -----
    For array x, the calculation is carried out as E[((x - mu) / sigma)^3],
    where E() is the expectation value, mu is the mean of x, and sigma is the
    uncorrected sample standard deviation.

    This is equivalent to
        >>> n = len(x)
        >>> numer = np.power(x - mu, 3).sum() / n
        >>> denom = np.power(np.power(x - mu, 2).sum() / (n - 1), 3 / 2)
        >>> skew = numer / denom
    and to the normalized 3rd-order central moment
        >>> mu_n(x, 3, normed=True)

    This function's output matches that of scipy.stats.skew exactly, but
    the latter is typically faster.

    """
    mean = x.mean(axis=None)
    sigma = x.std(ddof=0, axis=None)
    sigma = 1 if sigma <= 0 else sigma
    return np.mean(np.power((x - mean) / sigma, 3), axis=None)


def kurt(x, fisher=True):
    """Compute the kurtosis of an array as the fourth standardized moment.

    Parameters
    ----------
    x : array_like
        Input array. If `x` is multidimensional, kurtosis is automatically
        computed over all elements in the array, not just along one axis.
    fisher : bool, optional
        If True, use Fisher's normalization, i.e. subtract 3 from the result.

    Returns
    -------
    float
        Kurtosis of `x`.

    Notes
    -----
    For array x, the calculation is carried out as E[((x - mu) / sigma)^4],
    where E() is the expectation value, mu is the mean of x, and sigma is the
    uncorrected sample standard deviation.

    This is equivalent to the normalized 4th-order central moment
        >>> mu_n(x, 4, normed=True) - 3

    This function's output matches that of scipy.stats.kurtosis very well, but
    the latter is typically faster.

    """
    mean = x.mean(axis=None)
    sigma = x.std(ddof=0, axis=None)
    sigma = 1 if sigma <= 0 else sigma
    return np.mean(np.power((x - mean) / sigma, 4), axis=None) - [0, 3][fisher]


def mu_n(x, order, normed=False):
    """Compute the (normalized) nth-order central moment of a distribution.

    Parameters
    ----------
    x : array_like
        Input array. If `x` is multidimensional, the moment is automatically
        computed over all elements in the array, not just along one axis.
    order : int (positive)
        Order of the moment.
    normed : bool, optional
        If True, normalize the result by sigma^order, where sigma is the
        corrected sample standard deviation of `x`. Default is False.

    Returns
    -------
    float
        Nth-order moment of `x`.

    Notes
    -----
    This function's output matches that of scipy.stats.moment very well, but
    the latter is typically faster.

    """
    # Check order
    order = int(order)
    assert order > 0, "Only n > 0 supported."

    x = np.atleast_1d(x).flatten()
    mean = x.mean()

    if normed and order > 2:
        sigma = x.std(ddof=1)
        result = np.power((x - mean) / sigma, order).mean()
    else:
        result = np.power(x - mean, order).mean()

    return result


def kappa_n(x, order):
    """Compute the nth-order cumulant of a distribution.

    Parameters
    ----------
    x : array_like
        Input array. If `x` is multidimensional, the cumulant is automatically
        computed over all elements in the array, not just along one axis.
    order : int between 2 and 6, inclusive
        Order of the cumulant.

    Returns
    -------
    float
        Nth-order cumulant of `x`.

    Notes
    -----
    This function's output matches that of scipy.stats.kstat very well, but
    the latter is typically faster.

    """
    # Check order
    order = int(order)
    assert order in range(2, 7), "Order {} not supported.".format(order)

    if order == 2:
        kappa = mu_n(x, 2, normed=False)
    if order == 3:
        kappa = mu_n(x, 3, normed=False)
    if order == 4:
        mu2 = mu_n(x, 2, normed=False)
        mu4 = mu_n(x, 4, normed=False)
        kappa = mu4 - 3 * mu2**2
    if order == 5:
        mu2 = mu_n(x, 2, normed=False)
        mu3 = mu_n(x, 3, normed=False)
        mu5 = mu_n(x, 5, normed=False)
        kappa = mu5 - 10 * mu3 * mu2
    if order == 6:
        mu2 = mu_n(x, 2, normed=False)
        mu3 = mu_n(x, 3, normed=False)
        mu4 = mu_n(x, 4, normed=False)
        mu6 = mu_n(x, 6, normed=False)
        kappa = mu6 - 15 * mu4 * mu2 - 10 * mu3**2 + 30 * mu2**3

    return kappa


def fdr(x, tail, alpha=0.05, kde=False, n_samples=10000, debug=False):
    """Compute the false discovery rate (FDR) threshold of a distribution.

    Parameters
    ----------
    x : array_like
        Samples of the distribution. If `x` is multidimentional, it will
        first be flattened.
    tail : {'left', 'right'}
        Side of the distribution for which to compute the threshold.
    alpha : float, optional
        Maximum average false discovery rate. Default is 0.05.
    kde : bool, optional
        If True, compute p-values from the distribution smoothed by kernel
        density estimation. Not recommended for large `x`. Default is False.
    n_samples : int, optional
        Number of samples to draw if using KDE. Default is 10000.
    debug : bool, optional
        If True, print the number of detections and the final p-value.
        Default is False.

    Returns
    -------
    float
        FDR threshold.

    Examples
    --------
    ...

    """
    # Check inputs
    assert tail in ("left", "right"), "Invalid tail."

    # Basic measures
    x = np.atleast_1d(x).flatten()
    N = len(x)
    mean = x.mean()
    sigma = x.std(ddof=1)

    if kde:
        # Approximate the distribution by kernel density estimation
        pdf = gaussian_kde(x, bw_method="silverman")
        amin, amax = x.min(), x.max()
        vmax = np.sign(amax) * np.abs(amax + sigma)
        vmin = np.sign(amin) * np.abs(amin - sigma)

        # Cumulative distribution function
        def cdf(z, tail):
            if tail == "right":
                return pdf.integrate_box(-np.inf, z)
            else:
                return pdf.integrate_box(z, np.inf)

        # Inverse cumulative distribution function
        def cdfinv(t, tail):
            assert t > 0 and t < 1, "Input to cdfinv must be in (0, 1)."
            try:
                result = brentq(lambda x: cdf(x, tail) - t, vmin, vmax)
            except ValueError:
                print("Warning: bad value. Returning 0.")
                print("threshold = {}".format(t))
                print("vmin = {}".format(vmin))
                print("vmax = {}".format(vmax))
                result = 0
            return result

        # Compute p-values
        if tail == "right":
            smin = mean + sigma
            smax = vmax
        else:
            smax = mean - sigma
            smin = vmin
        samples = (smax - smin) * np.random.random(n_samples) + smin
        pvals = 1 - np.array([cdf(s, tail) for s in samples])
    else:
        # Compute p-values assuming Gaussian null hypothesis
        z = (x - mean) / sigma
        if tail == "right":
            pvals = 1 - norm.cdf(z)
        else:
            pvals = norm.cdf(z)

    # Sort p-values and find the last one
    inds = np.argsort(pvals)
    ialpha = alpha * np.arange(1, len(pvals) + 1) / float(len(pvals))
    diff = pvals[inds] - ialpha
    lastindex = np.argmax((diff < 0).cumsum())
    pval = pvals[inds][lastindex]
    if debug:
        print("{} detection(s) / {}".format(lastindex + 1, len(pvals)))
        print("last pval = {}".format(pval))

    # Invert pval to get threshold
    if kde:
        tval = cdfinv(1 - pval, tail)
    else:
        tval = x[inds][lastindex]

    return tval


def hc(x, kind=1):
    """Compute a higher criticism statistic of a distribution.

    Parameters
    ----------
    x : array_like
        Samples of the distribution. If `x` is multidimentional, it will
        first be flattened.
    kind : int, optional
        Either 1 (HC^*) or 2 (HC^+). Default is 1.

    Returns
    -------
    float
        Higher criticism of the first (*) or second (+) kind.

    References
    ----------
    * Donoho & Jin, The Annals of Statistics 32, 962 (2004)
    * Pires, Starck, Amara, et al., A&A 505, 969 (2009)

    Examples
    --------
    ...

    """
    # Check inputs
    assert kind in (1, 2), "Invalid kind. Must be either 1 or 2."

    # Basic measures
    x = np.atleast_1d(x).flatten()
    mean = x.mean()
    sigma = x.std(ddof=1)

    # Normalize values
    z = (x - mean) / sigma

    # Compute and sort p-values
    pvals = 1 - norm.cdf(z)
    pvals = np.sort(pvals)

    # Compute HC value
    denom2 = pvals - np.power(pvals, 2)
    good_inds = denom2 > 0  # Avoid division by zero and complex numbers
    pvals = pvals[good_inds]
    n = len(pvals)
    numer = np.sqrt(n) * (np.arange(1, n + 1) / float(n) - pvals)
    denom = np.sqrt(denom2[good_inds])
    ratio = numer / denom

    if kind == 2:
        ninv = 1.0 / n
        sel = (pvals >= ninv) & (pvals <= (1.0 - ninv))
        ratio = ratio[sel]

    return np.abs(ratio).max()


# function to generate a make a Gaussian Random field out of a gaussian spectrum
def get_grf(size, intput_power_map=None):
    """Create a Gaussian Random Field from a power spectrum.
       The created image has dimensions (size,size).
    Parameters
    ----------
    size: int
        Image size in each dimension
    intput_power_map : np array, optional
        input power spectrum

    Returns
    -------
    np array
        2D images.
    """
    # TODO: remove for loops for this code
    # TODO: check this code; it seems like
    #       `map_fourier[size - i, size - j] = np.conj(power_sample[i, j])`
    # is overwriting the line just above.
    k_map = np.zeros((size, size), dtype=float)
    power_map = np.zeros((size, size), dtype=float)
    for (i, j), val in np.ndenumerate(power_map):
        k1 = i - size / 2.0
        k2 = j - size / 2.0
        k_map[i, j] = np.sqrt(k1 * k1 + k2 * k2)
        if k_map[i, j] == 0:
            power_map[i, j] = 1e-15
        else:
            if power_map is None:
                power_map[i, j] = 1.0
            else:
                power_map[i, j] = intput_power_map[i, j]

    power_sample = np.random.normal(
        loc=0.0, scale=np.sqrt(0.5 * np.reshape(power_map, -1))
    ) + 1j * np.random.normal(loc=0.0, scale=np.sqrt(0.5 * np.reshape(power_map, -1)))
    power_sample = np.reshape(power_sample, (size, size))
    map_fourier = np.zeros((size, size), dtype=complex)

    for (
        i,
        j,
    ), value in np.ndenumerate(map_fourier):
        n = i - int(size / 2)
        m = j - int(size / 2)

        if n == 0 and m == 0:
            map_fourier[i, j] = 1e-15

        elif i == 0 or j == 0:
            map_fourier[i, j] = np.sqrt(2.0) * power_sample[i, j].real

        else:
            map_fourier[i, j] = power_sample[i, j]  #
            map_fourier[size - i, size - j] = np.conj(power_sample[i, j])

    map_pixel = np.fft.ifft2(np.fft.fftshift(map_fourier, axes=(-2, -1)))

    return map_pixel


def mad(input_data, axis=None, keepdims=False):
    r"""Median absolute deviation.

    This method calculates the median absolute deviation of the input data.
    Modification from modopt.math.stats

    Parameters
    ----------
    input_data : numpy.ndarray
        Input data array
    axis (int or tuple of ints)
        Axis or axes along which the mad is computed.
        The default is to compute the mad of the flattened array.
    keepdims (bool, optional)
        If this is set to True, the axes which are reduced are left in the
        result as dimensions with size one. With this option, the result will
        broadcast correctly against the original arr.

    Returns
    -------
    float
        MAD value

    Examples
    --------
    >>> import numpy as np
    >>> from modopt.math.stats import mad
    >>> a = np.arange(9).reshape(3, 3)
    >>> mad(a)
    2.0

    Notes
    -----
    The MAD is calculated as follows:

    .. math::

        \mathrm{MAD} = \mathrm{median}\left(|X_i - \mathrm{median}(X)|\right)

    See Also
    --------
    numpy.median : median function used

    """
    return np.median(
        np.abs(input_data - np.median(input_data, axis=axis, keepdims=True)),
        axis=axis,
        keepdims=keepdims,
    )
