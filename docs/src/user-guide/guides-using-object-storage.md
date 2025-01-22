# Using object storage

CLP can:

* [compress logs from object storage](#compressing-logs-from-object-storage) (e.g., S3);
* [store archives on object storage](#storing-archives-on-object-storage); and
* [view the compressed logs from object storage](#viewing-compressed-logs-from-object-storage).

This guide explains how to configure CLP for all three use cases. Note that you can choose to use
object storage for any combination of the three use cases (e.g., compress logs from S3 and view the
compressed logs from S3, but store archives on the local filesystem).

:::{note}
Currently, only the [clp-json][release-choices] release supports object storage. Support for
`clp-text` will be added in a future release.
:::

:::{note}
Currently, CLP only supports using S3 as object storage. Support for other object storage services
will be added in a future release.
:::

## Prerequisites

1. An S3 bucket and [key prefix][aws-key-prefixes] containing the logs you wish to compress.
    * An S3 URL is a combination of a bucket name and a key prefix as shown below:
      
      :::{mermaid}
      %%{
        init: {
          "theme": "base",
          "themeVariables": {
            "primaryColor": "#0066cc",
            "primaryTextColor": "#fff",
            "primaryBorderColor": "transparent",
            "lineColor": "#9580ff",
            "secondaryColor": "#9580ff",
            "tertiaryColor": "#fff"
          }
        }
      }%%
      graph TD
        A["s3://my-bucket-name/my-logs-dir/"] --"Bucket name"--> B[my-bucket-name]
        A --"Key prefix"--> C[path/to/my/file.txt]
      :::

2. An S3 bucket and key prefix where you wish to store compressed archives.
3. An S3 bucket and key prefix where you wish to store intermediate files for viewing compressed
   logs.
4. An AWS IAM user with the necessary permissions to access the S3 prefixes mentioned above.
    * To create a user, follow [this guide][aws-create-iam-user].
    * You may use a different IAM user for each use case to follow the
      [principle of least privilege][least-privilege-principle], or you can use the same user for
      all three.
    * For brevity, we'll refer to this user as the "CLP IAM user" in the rest of this guide.

:::{note}
CLP currently requires IAM user (long-term) credentials to access the relevant S3 buckets. Support
for other authentication methods (e.g., temporary credentials) will be added in a future release.
:::

## Compressing logs from object storage

To compress logs from S3, you'll need to:

1. Enable the CLP IAM user to access the S3 path containing your logs.
2. Use the `s3` subcommand of `sbin/compress.sh` to compress your logs.

### IAM user configuration

Attach the following policy to the CLP IAM user by following [this guide][add-iam-policy].

```json
{
   "Version": "2012-10-17",
   "Statement": [
       {
           "Effect": "Allow",
           "Action": "s3:GetObject",
           "Resource": [
               "arn:aws:s3:::<bucket-name>/<key-prefix>/*"
           ]
       },
       {
           "Effect": "Allow",
           "Action": [
               "s3:ListBucket"
           ],
           "Resource": [
               "arn:aws:s3:::<bucket-name>"
           ],
           "Condition": {
               "StringLike": {
                   "s3:prefix": "<key-prefix>/*"
               }
           }
       }
   ]
}
```

Replace the fields in angle brackets (`<>`) with the appropriate values:

* `<bucket-name>` should be the name of the S3 bucket containing your logs.
* `<key-prefix>` should be the prefix of all logs you wish to compress.

### Using `sbin/compress.sh s3`

You can use the `s3` subcommand as follows:

```bash
sbin/compress.sh s3 --aws-credentials-file <credentials-file> s3://<bucket-name>/<key-prefix>
```

* `<credentials-file>` is the path to an AWS credentials file like the following:

    ```ini
    [default]
    aws_access_key_id = <aws-access-key-id>
    aws_secret_access_key = <aws-secret-access-key>
    ```

    * CLP expects the credentials to be in the `default` section.
    * `<aws-access-key-id>` and `<aws-secret-access-key>` are the access key ID and secret access
      key of the CLP IAM user.
    * If you don't want to use a credentials file, you can specify the credentials on the command
      line using the `--aws-access-key-id` and `--aws-secret-access-key` flags.

* `<bucket-name>` is the name of the S3 bucket containing your logs.
* `<key-prefix>` is the prefix of all logs you wish to compress.

:::{note}
The `s3` subcommand only supports a single URL but will compress any logs that have the given
prefix.

If you wish to compress a single log file, specify the entire path to the log file. However, if that
log file's path is a prefix of another log file's path, then both log files will be compressed. This
limitation will be addressed in a future release.
:::

## Storing archives on object storage

To store compressed archives on S3, you'll need to:

1. Enable the CLP IAM user to access the S3 path where archives should be stored.
2. Configure CLP to store archives under the relevant S3 path.

### IAM user configuration

Attach the following policy to the CLP IAM user by following [this guide][add-iam-policy].

```json
{
  "Version": "2012-10-17",
  "Statement": [
   {
    "Effect": "Allow",
    "Action": [
      "s3:GetObject",
      "s3:PutObject"
    ],
    "Resource": [
      "arn:aws:s3:::<bucket-name>/<key-prefix>/*"
    ]
   }
  ]
}
```

Replace the fields in angle brackets (`<>`) with the appropriate values:

* `<bucket-name>` should be the name of the S3 bucket to store compressed archives.
* `<key-prefix>` should be the prefix where you want the compressed archives to be stored under.

### Configuring CLP's archive storage location

To configure CLP to store archives on S3, update the `archive_output.storage` key in
`<package>/etc/clp-config.yml`:

```yaml
archive_output:
  storage:
    type: "s3"
    staging_directory: "var/data/staged-archives"  # Or a path of your choosing
    s3_config:
      region_code: "<region-code>"
      bucket: "<bucket-name>"
      key_prefix: "<key-prefix>"
      credentials:
        access_key_id: "<aws-access-key-id>"
        secret_access_key: "<aws-secret-access-key>"

  # archive_output's other config keys
```

Replace the fields in angle brackets (`<>`) with the appropriate values:

* `s3_config` configures both the S3 bucket where archives should be stored and the credentials
  for accessing it.
  * `<region-code>` is the AWS region [code][aws-region-codes] for the bucket.
  * `<bucket-name>` is the bucket's name.
  * `<key-prefix>` is the "directory" where all archives will be stored within the bucket and
    must end with `/`.
  * `credentials` contains the CLP IAM user's credentials.

## Viewing compressed logs from object storage

To view compressed logs from S3, you'll need to:

1. Enable the CLP IAM user to access the S3 path where stream files (logs in a format viewable by
   the log viewer) should be stored.
2. Set up a cross-origin resource sharing (CORS) policy for the S3 path in (1).
3. Configure CLP to store stream files under the S3 path in (1).

### IAM user configuration

Attach the following policy to the CLP IAM user by following [this guide][add-iam-policy].

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "s3:GetObject",
        "s3:PutObject"
      ],
      "Resource": [
        "arn:aws:s3:::<bucket-name>/<key-prefix>/*"
      ]
    }
  ]
}
```

### Cross-origin resource sharing (CORS) configuration

For CLP's log viewer to be able to view the compressed logs from S3 over the internet, the S3 bucket
must have a CORS policy configured.

Add the CORS configuration below to your bucket by following [this guide][aws-cors-guide]:

```json
[
  {
    "AllowedHeaders": [
      "*"
    ],
    "AllowedMethods": [
      "GET"
    ],
    "AllowedOrigins": [
      "*"
    ],
    "ExposeHeaders": [
      "Access-Control-Allow-Origin"
    ]
  }
]
```

:::{tip}
The CORS policy above allows requests from any host (origin). If you already know what hosts will
access CLP's web interface, you can enhance security by changing `AllowedOrigins` from `*` to the
specific list of hosts that will access the web interface.
:::

### Configuring CLP's stream storage location

To configure CLP to store stream files on S3, update the `stream_output.storage` key in
`<package>/etc/clp-config.yml`:

```yaml
stream_output:
  storage:
    type: "s3"
    staging_directory: "var/data/staged-streams"  # Or a path of your choosing
    s3_config:
      region_code: "<region-code>"
      bucket: "<bucket-name>"
      key_prefix: "<key-prefix>"
      credentials:
        access_key_id: "<aws-access-key-id>"
        secret_access_key: "<aws-secret-access-key>"

  # stream_output's other config keys
```

The configuration keys above function identically to those in `archive_output.storage`, except they
should be configured to use a different S3 path.

:::{note}
To view compressed log files, clp-text currently converts them into IR streams that the log viewer
can open, while clp-json converts them into JSONL streams. These streams only need to be stored for
as long as the streams are being viewed, but CLP currently doesn't explicitly delete the streams.
This limitation will be addressed in a future release.
:::

[add-iam-policy]: https://docs.aws.amazon.com/IAM/latest/UserGuide/access_policies_manage-attach-detach.html#embed-inline-policy-console
[aws-cors-guide]: https://docs.aws.amazon.com/AmazonS3/latest/userguide/enabling-cors-examples.html
[aws-create-iam-user]: https://docs.aws.amazon.com/IAM/latest/UserGuide/id_users_create.html
[aws-key-prefixes]: https://docs.aws.amazon.com/AmazonS3/latest/userguide/using-prefixes.html
[aws-region-codes]: https://docs.aws.amazon.com/AmazonRDS/latest/UserGuide/Concepts.RegionsAndAvailabilityZones.html#Concepts.RegionsAndAvailabilityZones.Availability
[least-privilege-principle]: https://en.wikipedia.org/wiki/Principle_of_least_privilege
[release-choices]: quick-start-cluster-setup/index.md#choosing-a-release
