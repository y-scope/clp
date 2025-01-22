# Viewing compressed logs

To view compressed logs from S3, you'll need to:

1. Enable the CLP IAM user to access the S3 path where stream files (logs in a format viewable by
   the log viewer) should be stored.
2. Set up a cross-origin resource sharing (CORS) policy for the S3 path in (1).
3. Configure CLP to store stream files under the S3 path in (1).

## IAM user configuration

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

## Cross-origin resource sharing (CORS) configuration

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

## Configuring CLP's stream storage location

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

[aws-cors-guide]: https://docs.aws.amazon.com/AmazonS3/latest/userguide/enabling-cors-examples.html
[add-iam-policy]: https://docs.aws.amazon.com/IAM/latest/UserGuide/access_policies_manage-attach-detach.html#embed-inline-policy-console
