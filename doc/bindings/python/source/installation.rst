..
   SPDX-FileCopyrightText: 2020 Philippe Proulx <eeppeliteloop@gmail.com>
   SPDX-License-Identifier: CC-BY-SA-4.0

.. include:: common.rst

Installation
============
Linux distributions
-------------------
Linux distributions typically provide the Babeltrace |~| 2 Python
bindings as the ``python3-bt2`` package.

Build and install from source
-----------------------------
When you
`build Babeltrace 2 from source <https://babeltrace.org/#bt2-build-from-src>`_,
Python bindings and Python plugin support are enabled by default,
for example:

.. code-block:: text

   $ ./configure

See the project's
:bt2link:`README <https://github.com/efficios/babeltrace/blob/stable-@ver@/README.adoc>`
for build-time requirements and detailed build instructions.

.. note::

   The Babeltrace |~| 2 Python bindings only work with Python |~| 3.
