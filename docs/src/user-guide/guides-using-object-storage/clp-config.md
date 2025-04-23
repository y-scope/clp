# Configuring CLP

To use object storage with CLP, follow the steps below to configure each use case you require.

:::{note}
If CLP is already running, shut it down, update its configuration, and then start it again.
:::

## Configuration for input logs

To configure CLP to compress logs from S3, update the `logs_input` key in
`<package>/etc/clp-config.yml` with the values in the code block below, replacing the fields in
angle brackets (`<>`) with the appropriate values:

```yaml
logs_input:
  type: "s3"
  s3_config:
    region_code: "<region-code>"
    bucket: "<bucket-name>"
    key_prefix: "<key-prefix>"
    aws_authentication:
      type: "<authentication-type>"
      profile: "<aws-config-profile>" # Only for type "profile"
      credentials: # Only for type "credentials"
        access_key_id: "<aws-access-key-id>"
        secret_access_key: "<aws-secret-access-key>"
```
* `s3_config` configures both the S3 bucket where logs are to be retrieved from and the credentials
  for accessing it.
  * `<region-code>` is the AWS region [code][aws-region-codes] for the bucket.
  * `<bucket-name>` is the bucket's name.
  * `<key-prefix>` is the prefix of all logs you wish to compress and should be the same as the
    `<all-logs-prefix>` value from the [compression IAM policy][compression-iam-policy].
  * `aws_authentication` configures the type of authentication used and additional information if
    needed.
    * `type` must be one of `credentials`, `profile`, `env_vars`, and `ec2`.
    * If using long-term IAM credentials, they should be provided under the `credentials` field.
    * If using a named profile, the name should be provided under the `profile` field.
    * If using credentials set as environmental variables or running CLP on an EC2 instance with
      IAM role attached, no other information is needed.

## Configuration for archive storage

To configure CLP to store archives on S3, update the `archive_output.storage` key in
`<package>/etc/clp-config.yml` with the values in the code block below, replacing the fields in
angle brackets (`<>`) with the appropriate values:

```yaml
archive_output:
  storage:
    type: "s3"
    staging_directory: "var/data/staged-archives"  # Or a path of your choosing
    s3_config:
      region_code: "<region-code>"
      bucket: "<bucket-name>"
      key_prefix: "<key-prefix>"
      aws_authentication:
        type: "<authentication-type>"
        profile: "<aws-config-profile>" # Only for type "profile"
        credentials: # Only for type "credentials"
          access_key_id: "<aws-access-key-id>"
          secret_access_key: "<aws-secret-access-key>"

  # archive_output's other config keys
```

* `staging_directory` is the local filesystem directory where archives will be temporarily stored
  before being uploaded to S3.
* `s3_config` configures both the S3 bucket where archives should be stored and the credentials
  for accessing it.
  * `<region-code>` is the AWS region [code][aws-region-codes] for the bucket.
  * `<bucket-name>` is the bucket's name.
  * `<key-prefix>` is the "directory" where all archives will be stored within the bucket and
    must end with a trailing forward slash (e.g., `archives/`).
  * `aws_authentication` configures the type of authentication used and additional information if
    needed.
    * `type` must be one of `credentials`, `profile`, `env_vars`, and `ec2`.
    * If using long-term IAM credentials, they should be provided under the `credentials` field.
    * If using a named profile, the name should be provided under the `profile` field.
    * If using credentials set as environmental variables or running CLP on an EC2 instance with
      IAM role attached, no other information is needed.

## Configuration for stream storage

To configure CLP to cache stream files on S3, update the `stream_output.storage` key in
`<package>/etc/clp-config.yml` with the values in the code block below, replacing the fields in
angle brackets (`<>`) with the appropriate values:

```yaml
stream_output:
  storage:
    type: "s3"
    staging_directory: "var/data/staged-streams"  # Or a path of your choosing
    s3_config:
      region_code: "<region-code>"
      bucket: "<bucket-name>"
      key_prefix: "<key-prefix>"
      aws_authentication:
        type: "<authentication-type>"
        profile: "<aws-config-profile>" # Only for type "profile"
        credentials: # Only for type "credentials"
          access_key_id: "<aws-access-key-id>"
          secret_access_key: "<aws-secret-access-key>"

  # stream_output's other config keys
```

* `staging_directory` is the local filesystem directory where streams will be temporarily stored
  before being uploaded to S3.
* `s3_config` configures both the S3 bucket where streams should be stored and the credentials
  for accessing it.
  * `<region-code>` is the AWS region [code][aws-region-codes] for the bucket.
  * `<bucket-name>` is the bucket's name.
  * `<key-prefix>` is the "directory" where all streams will be stored within the bucket and
    must end with a trailing forward slash (e.g., `streams/`).
  * `aws_authentication` configures the type of authentication used and additional information if
    needed.
    * `type` must be one of `credentials`, `profile`, `env_vars`, and `ec2`.
    * If using long-term IAM credentials, they should be provided under the `credentials` field.
    * If using a named profile, the name should be provided under the `profile` field.
    * If using credentials set as environmental variables or running CLP on an EC2 instance with
      IAM role attached, no other information is needed.

:::{note}
CLP currently doesn't explicitly delete the cached streams. This limitation will be addressed in a
future release.
:::

## Configuration for AWS config directory

If using the authentication type `profile` in any of the components above, CLP must be configured
with the directory in which AWS configuration are found. Typically, this should be the `.aws`
folder in the user's home directory.

```yaml
aws_config_directory: "/home/<clp-user>/.aws"
```

If profiles are not used for AWS authentication, the field should be left empty.

[aws-region-codes]: https://docs.aws.amazon.com/AmazonRDS/latest/UserGuide/Concepts.RegionsAndAvailabilityZones.html#Concepts.RegionsAndAvailabilityZones.Availability
[compression-iam-policy]: ./object-storage-config.md#configuration-for-compression
