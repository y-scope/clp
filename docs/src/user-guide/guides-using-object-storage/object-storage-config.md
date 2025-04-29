# Configuring object storage

To use object storage with CLP, follow the steps below to configure the necessary IAM permissions
and your object storage bucket(s) for each use case you require.

## Configuration for compression

[Attach the policy][add-iam-policy] (managed or inline) below to the IAM user, role, or
[permission set][aws-permission-sets] that CLP will use (you can use the JSON editor), replacing
the fields in angle brackets (`<>`) with the appropriate values:

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "s3:GetObject",
      "Resource": [
        "arn:aws:s3:::<bucket-name>/<all-logs-prefix>*"
      ]
    },
    {
      "Effect": "Allow",
      "Action": "s3:ListBucket",
      "Resource": [
        "arn:aws:s3:::<bucket-name>"
      ],
      "Condition": {
        "StringLike": {
          "s3:prefix": "<all-logs-prefix>*"
        }
      }
    }
  ]
}
```

* `<bucket-name>` should be the name of the S3 bucket containing your logs.
* `<all-logs-prefix>` should be the prefix of all logs you wish to compress.

  :::{note}
  If you want to enforce that only logs under a directory-like prefix, e.g., `logs/`, can be
  compressed, you can append a trailing slash (`/`) after the `<all-logs-prefix>` value. This will
  prevent CLP from compressing logs with prefixes like `logs-private`. However, note that to
  compress all logs under the `logs/` prefix, you will need to include the trailing slash when
  invoking `sbin/compress.sh` below.
  :::

## Configuration for archive storage

[Attach the policy][add-iam-policy] (managed or inline) below to the IAM user, role, or
[permission set][aws-permission-sets] that CLP will use (you can use the JSON editor), replacing
the fields in angle brackets (`<>`) with the appropriate values:

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

* `<bucket-name>` should be the name of the S3 bucket where compressed archives should be stored.
* `<key-prefix>` should be the prefix (used like a directory path) where compressed archives should
  be stored.

## Configuration for stream storage

The [log viewer][yscope-log-viewer] currently supports viewing [IR][uber-clp-blog-1] and JSONL
stream files but not CLP archives; thus, to view the compressed logs from a CLP archive, CLP first
converts the compressed logs into stream files. These streams can be cached on the filesystem, or on
object storage.

:::{note}
A future version of the log viewer will support viewing CLP archives directly.
:::

Storing streams on S3 requires both configuring the CLP IAM user and setting up a cross-origin
resource sharing (CORS) policy for the S3 bucket.

### IAM user configuration

[Attach the policy][add-iam-policy] (managed or inline) below to the IAM user, role, or
[permission set][aws-permission-sets] that CLP will use (you can use the JSON editor), replacing
the fields in angle brackets (`<>`) with the appropriate values:

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

* `<bucket-name>` should be the name of the S3 bucket where cached streams should be stored.
* `<key-prefix>` should be the prefix (used like a directory path) where cached streams should be
  stored.

### Cross-origin resource sharing (CORS) configuration

For CLP's log viewer to be able to access the cached stream files from S3 over the internet, the S3
bucket must have a CORS policy configured.

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
access CLP's web interface, you can enhance security by changing `AllowedOrigins` from `["*"]` to
the specific list of hosts that will access the web interface.
:::

[aws-cors-guide]: https://docs.aws.amazon.com/AmazonS3/latest/userguide/enabling-cors-examples.html
[aws-permission-sets]: https://docs.aws.amazon.com/singlesignon/latest/userguide/permissionsetsconcept.html
[add-iam-policy]: https://docs.aws.amazon.com/IAM/latest/UserGuide/access_policies_manage-attach-detach.html#embed-inline-policy-console
[uber-clp-blog-1]: https://www.uber.com/en-US/blog/reducing-logging-cost-by-two-orders-of-magnitude-using-clp
[yscope-log-viewer]: https://github.com/y-scope/yscope-log-viewer
