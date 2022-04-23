# meter-network-simulator

Simulate an Advanced Metering Infrastructure (AMI) network.

This is a self-contained version of [ns-3](https://www.nsnam.org/) that is intended to be used as a base with which to model AMI networks.  It builds and runs in a [podman](https://podman.io) or [Docker](https://www.docker.com) container.

The full manual for this software is available from EPRI:  https://www.epri.com/research/products/000000003002024109

## Building
### Note:
> Although these instructions use `podman`, either `podman` or `docker` may be used.  Simply substitude `docker` for `podman` in each of the following sections.

The simplest way to build this container, is from the command line.  With the `Dockerfile` in an empty subdirectory, use this command:

```
podman build --target builder -t epri/ns3 .
```

This will create and a code-only version of the container which will be called `epri/ns3`.  To create a version that builds and includes all of the ns-3 documentation, including tutorial, manual and code documentation, use this:

```
podman build -t epri/ns3-withdocs .
```

The `epri/ns3-withdocs` image is much larger (about 11G, as compared with 1.4G for the `epri/ns3` version) and most of the documentation is available via the [ns-3 website](https://www.nsnam.org/documentaion).  The version with the documentation may be useful for those who wish to use the simulator while not having a connection to the internet, but both version require an internet connection to build.

Also be aware that even on a fast multi-core computer with a gigabit fiber internet access, a full build (with documentation) takes over an hour.  The code-only version takes about half that time.

## Running
With the container built and run, we can use `ns-3` to run various simulations.  For example, we can try running a sample model which simulates two `lr-wpan` nodes attempting to communicate at various distances.

```
podman run -v "$(pwd)/epri":/tmp/ns-3-dev/scratch/epri:z epri/ns3 epri/lr-wpan-error-distance-plot-epri.cc
```

This will compile and run the `lr-wpan-error-distance-plot-epri.cc` file which is a minor adaptation of a file that comes with ns-3.  In particular, it is part of the suite of tests in the [LR-WPAN model](https://www.nsnam.org/docs/release/3.37/models/html/lr-wpan.html), which is a simulation of the RF protocol specified in standard IEEE 802.15.4 which is commonly employed for AMI networks.

The result of running this program is a file in the `epri` subdirectory named `802.15.4-psr-distance.plt`.  This is a [gnuplot](http://www.gnuplot.info/) file which will create an Encapsulated PostScript (EPS) file if run with this command:

```
gnuplot epri/802.15.4-psr-distance.plt
```

Note that this command is run on the *host* Linux computer and not within the container, so gnuplot must be installed on the host computer.  To view the file, one can use [`evince`](https://wiki.gnome.org/Apps/Evince) or any other software capable of rendering EPS files.  That file should look something like this:

![gnuplot output](/images/802.15.4-psr-distance.png?raw=true "Sample graphical output")

## Advanced usage
A container is convenient and useful to avoid having to do a lot of manual configuration, but for those interested in modifying or adapting the models, it is definitely faster to run the simulator natively on the host Fedora Linux machine.  To do so, install `ns-3` per the instructions on the tool's [web site](https://www.nsnam.org/), copy the contents of the `epri` directory from this project to the `scratch` directory within the newly installed `ns-3` installation and enjoy experimenting!
