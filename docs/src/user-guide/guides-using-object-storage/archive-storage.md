# Storing archives

To store compressed archives on S3, you'll need to:

1. Enable the CLP IAM user to access the S3 path where archives should be stored.
2. Configure CLP to store archives under the relevant S3 path.

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

Replace the fields in angle brackets (`<>`) with the appropriate values:

* `<bucket-name>` should be the name of the S3 bucket to store compressed archives.
* `<key-prefix>` should be the prefix where you want the compressed archives to be stored under.

## Configuring CLP's archive storage location

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

* `staging_directory` is the local filesystem directory where archives will be temporarily stored
  before being uploaded to S3.
* `s3_config` configures both the S3 bucket where archives should be stored and the credentials
  for accessing it.
  * `<region-code>` is the AWS region [code][aws-region-codes] for the bucket.
  * `<bucket-name>` is the bucket's name.
  * `<key-prefix>` is the "directory" where all archives will be stored within the bucket and
    must end with `/`.
  * `credentials` contains the CLP IAM user's credentials.

[add-iam-policy]: https://docs.aws.amazon.com/IAM/latest/UserGuide/access_policies_manage-attach-detach.html#embed-inline-policy-console
[aws-region-codes]: https://docs.aws.amazon.com/AmazonRDS/latest/UserGuide/Concepts.RegionsAndAvailabilityZones.html#Concepts.RegionsAndAvailabilityZones.Availability
