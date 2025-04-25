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
  aws_authentication:
    type: "<type>"
    # type-specific settings
```
* `<type>` and the type-specific settings are described in the
  [configuring AWS authentication](#configuring-aws-authentication) section.

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
        type: "<type>"
        # type-specific settings

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
  * `<type>` and the type-specific settings are described in the
    [configuring AWS authentication](#configuring-aws-authentication) section.

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
        type: "<type>"
        # type-specific settings

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
  * `<type>` and the type-specific settings are described in the
    [configuring AWS authentication](#configuring-aws-authentication) section.

:::{note}
CLP currently doesn't explicitly delete the cached streams. This limitation will be addressed in a
future release.
:::

## Configuring AWS authentication

For each use case above, you can configure its AWS authentication method through the
`aws_authentication` config object, which includes the authentication method type to use and any
additional settings necessary for the chosen authentication type.

:::{note}
The code blocks below show `aws_authentication` as a top-level key, but it should be nested under
`logs_input.s3_config`, `archive_output.storage.s3_config`, or `stream_output.storage.s3_config`
depending on the use case.
:::

Settings for each type are described below:

* [credentials](#credentials)
* [profile](#profile)
* [env_vars](#env_vars)
* [ec2](#ec2)

### credentials

Settings for this type are shown below. Replace fields in angle brackets (`<>`) with the appropriate
values:

```yaml
aws_authentication:
  type: "credentials"
  credentials:
    access_key_id: "<aws-access-key-id>"
    secret_access_key: "<aws-secret-access-key>"
```

`<aws-access-key-id>` and `<aws-secret-access-key>` should be replaced with
[long-term credentials](index.md#long-term-iam-user-credentials) for an IAM user.

### profile

Settings for this type are shown below. Replace fields in angle brackets (`<>`) with the appropriate
values:

```yaml
aws_authentication:
  type: "profile"
  profile: "<profile-name>"
```

`<profile-name>` should be the name of an existing [AWS CLI profile](index.md#named-profiles).

In addition, the _top-level_ config `aws_config_directory` must be set to the directory containing
the profile configurations (typically `~/.aws`):

```yaml
aws_config_directory: "<aws-config-dir>"
```

:::{note}
If profiles are not used for AWS authentication, `aws_config_directory` should be commented or set
to `null`.
:::

### env_vars

Settings for this type are shown below.

```yaml
aws_authentication:
  type: "env_vars"
```

The environment variables `AWS_ACCESS_KEY_ID` and `AWS_SECRET_ACCESS_KEY` should be used to specify
a set of [long-term IAM user credentials](index.md#long-term-iam-user-credentials).

### ec2

Settings for this type are shown below.

```yaml
aws_authentication:
  type: "ec2"
```

This authentication method will only work on an EC2 instance with a
[role attached](index.md#ec2-instance-iam-roles).

[aws-region-codes]: https://docs.aws.amazon.com/AmazonRDS/latest/UserGuide/Concepts.RegionsAndAvailabilityZones.html#Concepts.RegionsAndAvailabilityZones.Availability
[compression-iam-policy]: ./object-storage-config.md#configuration-for-compression
