# Datasets

This page lists a few large log datasets you can use to try out CLP and evaluate
its compression ratio against other tools. Each dataset is gzipped for more
efficient downloads. We will be uploading more datasets over time.

For evaluation results comparing CLP and other tools, see our
[paper](https://www.usenix.org/system/files/osdi21-rodrigues.pdf).

| Dataset                            | Format | Uncompressed size | Download size |
|------------------------------------|--------|-------------------|---------------|
| [hadoop-14TB-part1<sup>†</sup>][1] | Text   | 428.94 GB         | 20.33 GB      |
| [openstack-24hr][2]                | Text   | 33.00 GB          | 2.06 GB       |
| [hive-24hr][3]                     | Text   | 2.07 GB           | 122.54 MB     |
| [mongodb][4]                       | JSON   | 64.80 GB          | 1.48 GB       |
| [cockroachdb][5]                   | JSON   | 9.79 GB           | 528.97 MB     |
| [elasticsearch][6]                 | JSON   | 7.98 GB           | 165.91 MB     |
| [spark-event-logs][7]              | JSON   | 1.98 GB           | 211.88 MB     |

*<sup>†</sup> We will upload the other parts soon.*

[1]: https://zenodo.org/records/7114846

[2]: https://zenodo.org/records/7094971

[3]: https://zenodo.org/records/7094920

[4]: https://zenodo.org/records/10516284

[5]: https://zenodo.org/records/10516386

[6]: https://zenodo.org/records/10516226

[7]: https://zenodo.org/records/10516345
