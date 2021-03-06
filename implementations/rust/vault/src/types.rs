use crate::error::{VaultFailError, VaultFailErrorKind};
#[cfg(feature = "ffi")]
use ffi_support::IntoFfi;
use subtle::ConstantTimeEq;
use zeroize::Zeroize;

/// The types of secret keys that the vault supports
#[derive(Copy, Clone, Debug, Hash, Eq, PartialEq, Ord, PartialOrd, Zeroize)]
pub enum SecretKeyType {
    /// Raw buffer of bytes
    Buffer(usize),
    /// AES-128 bit key
    Aes128,
    /// AES-256 bit key
    Aes256,
    /// x25519 secret key
    Curve25519,
    /// NIST P-256 (secp256r1, prime256v1) secret key
    P256,
}

impl SecretKeyType {
    /// Convert enum to a number
    pub fn to_usize(&self) -> usize {
        match *self {
            SecretKeyType::Buffer(..) => 0,
            SecretKeyType::Aes128 => 1,
            SecretKeyType::Aes256 => 2,
            SecretKeyType::Curve25519 => 3,
            SecretKeyType::P256 => 4,
        }
    }

    /// Try to convert from a number to the rust enum
    pub fn from_usize(value: usize) -> Result<Self, VaultFailError> {
        match value {
            0 => Ok(SecretKeyType::Buffer(0)),
            1 => Ok(SecretKeyType::Aes128),
            2 => Ok(SecretKeyType::Aes256),
            3 => Ok(SecretKeyType::Curve25519),
            4 => Ok(SecretKeyType::P256),
            _ => Err(VaultFailErrorKind::InvalidParam(0).into()),
        }
    }
}

from_int_impl!(SecretKeyType, i8);
from_int_impl!(SecretKeyType, i16);
from_int_impl!(SecretKeyType, i32);
from_int_impl!(SecretKeyType, i64);
from_int_impl!(SecretKeyType, i128);
from_int_impl!(SecretKeyType, u8);
from_int_impl!(SecretKeyType, u16);
from_int_impl!(SecretKeyType, u32);
from_int_impl!(SecretKeyType, u64);
from_int_impl!(SecretKeyType, u128);
try_from_int_impl!(SecretKeyType, i8);
try_from_int_impl!(SecretKeyType, i16);
try_from_int_impl!(SecretKeyType, i32);
try_from_int_impl!(SecretKeyType, i64);
try_from_int_impl!(SecretKeyType, i128);
try_from_int_impl!(SecretKeyType, u8);
try_from_int_impl!(SecretKeyType, u16);
try_from_int_impl!(SecretKeyType, u32);
try_from_int_impl!(SecretKeyType, u64);
try_from_int_impl!(SecretKeyType, u128);

/// Persistence allowed by Secrets
#[derive(Copy, Clone, Debug, Hash, Ord, PartialOrd, Eq, PartialEq, Zeroize)]
pub enum SecretPersistenceType {
    /// Secret is temporary
    Ephemeral,
    /// Secret is permanent
    Persistent,
}

impl SecretPersistenceType {
    /// Convert enum to a number
    pub fn to_usize(&self) -> usize {
        match *self {
            SecretPersistenceType::Ephemeral => 0,
            SecretPersistenceType::Persistent => 1,
        }
    }

    /// Try to convert from a number to the rust enum
    pub fn from_usize(value: usize) -> Result<Self, VaultFailError> {
        match value {
            0 => Ok(SecretPersistenceType::Ephemeral),
            1 => Ok(SecretPersistenceType::Persistent),
            _ => Err(VaultFailErrorKind::InvalidParam(0).into()),
        }
    }
}

from_int_impl!(SecretPersistenceType, i8);
from_int_impl!(SecretPersistenceType, i16);
from_int_impl!(SecretPersistenceType, i32);
from_int_impl!(SecretPersistenceType, i64);
from_int_impl!(SecretPersistenceType, i128);
from_int_impl!(SecretPersistenceType, u8);
from_int_impl!(SecretPersistenceType, u16);
from_int_impl!(SecretPersistenceType, u32);
from_int_impl!(SecretPersistenceType, u64);
from_int_impl!(SecretPersistenceType, u128);
try_from_int_impl!(SecretPersistenceType, i8);
try_from_int_impl!(SecretPersistenceType, i16);
try_from_int_impl!(SecretPersistenceType, i32);
try_from_int_impl!(SecretPersistenceType, i64);
try_from_int_impl!(SecretPersistenceType, i128);
try_from_int_impl!(SecretPersistenceType, u8);
try_from_int_impl!(SecretPersistenceType, u16);
try_from_int_impl!(SecretPersistenceType, u32);
try_from_int_impl!(SecretPersistenceType, u64);
try_from_int_impl!(SecretPersistenceType, u128);

/// Secrets specific purpose
#[derive(Copy, Clone, Debug, Hash, Ord, PartialOrd, Eq, PartialEq, Zeroize)]
pub enum SecretPurposeType {
    /// Key exchange
    KeyAgreement,
}

impl SecretPurposeType {
    /// Convert enum to a number
    pub fn to_usize(&self) -> usize {
        match *self {
            SecretPurposeType::KeyAgreement => 0,
        }
    }

    /// Try to convert from a number to the rust enum
    pub fn from_usize(value: usize) -> Result<Self, VaultFailError> {
        match value {
            0 => Ok(SecretPurposeType::KeyAgreement),
            _ => Err(VaultFailErrorKind::InvalidParam(0).into()),
        }
    }
}

from_int_impl!(SecretPurposeType, i8);
from_int_impl!(SecretPurposeType, i16);
from_int_impl!(SecretPurposeType, i32);
from_int_impl!(SecretPurposeType, i64);
from_int_impl!(SecretPurposeType, i128);
from_int_impl!(SecretPurposeType, u8);
from_int_impl!(SecretPurposeType, u16);
from_int_impl!(SecretPurposeType, u32);
from_int_impl!(SecretPurposeType, u64);
from_int_impl!(SecretPurposeType, u128);
try_from_int_impl!(SecretPurposeType, i8);
try_from_int_impl!(SecretPurposeType, i16);
try_from_int_impl!(SecretPurposeType, i32);
try_from_int_impl!(SecretPurposeType, i64);
try_from_int_impl!(SecretPurposeType, i128);
try_from_int_impl!(SecretPurposeType, u8);
try_from_int_impl!(SecretPurposeType, u16);
try_from_int_impl!(SecretPurposeType, u32);
try_from_int_impl!(SecretPurposeType, u64);
try_from_int_impl!(SecretPurposeType, u128);

/// Attributes for a specific vault secret
#[derive(Copy, Clone, Debug, Hash, Ord, PartialOrd, Eq, PartialEq, Zeroize)]
pub struct SecretKeyAttributes {
    /// The type of key
    pub xtype: SecretKeyType,
    /// How the key is persisted
    pub persistence: SecretPersistenceType,
    /// The purpose of the secret key
    pub purpose: SecretPurposeType,
}

/// A context that uses secret keys e.g. TEE, HSM, SEP.
/// This list is not meant to be exhaustive, just the ones supported
/// Ockam vault.
///
/// Future options include:
///
/// Key is backed by Amazon's CloudHSM
/// AmazonCloudHsm,
/// Key is backed by Amazon's KMS
/// AmazonKMS,
/// Key is backed by Box's Keysafe
/// BoxKeysafe,
/// Key is backed HashiCorp KeyVault
/// HashiCorpVault,
/// Key is backed by an Intel SGX enclave
/// IntelSgx,
/// Key is backed by an AMD's PSP enclave
/// AmdPsp,
/// Key is backed by an ARM's Trustzone enclave
/// ArmTrustzone,
/// Key is backed a Apple's secure enclave
/// SecureEnclave,
#[derive(Clone, Copy, Debug, Hash, Ord, PartialOrd, Eq, PartialEq, Zeroize)]
pub enum SecretKeyContext {
    /// Key is backed by RAM
    Memory(usize),
    /// Key is backed by a file
    File,
    /// Key is backed Azure KeyVault
    AzureKeyVault,
    /// Key is backed an OS keyring like Windows Credential Vault, Gnome Keyring, KWallet, or
    /// Security Framework
    KeyRing {
        /// The id of the secret
        id: usize,
        /// The type of represented by `id`
        os_type: OsKeyRing,
    },
}

/// The type of OS represented by the Keyring
#[derive(Clone, Copy, Debug, Hash, Ord, PartialOrd, Eq, PartialEq, Zeroize)]
pub enum OsKeyRing {
    /// The keyring tied to a Linux operating system
    Linux,
    /// The keyring tied to the Mac OS X operating system
    Osx(OsxContext),
    /// The credentials vault tied to the Windows operating system
    Windows,
}

/// Locations that can be used by the OSX system
#[derive(Clone, Copy, Debug, Hash, Ord, PartialOrd, Eq, PartialEq, Zeroize)]
pub enum OsxContext {
    /// Secrets stored ephemerally in memory
    Memory,
    /// Secrets stored in the OSX keychain
    Keychain,
    /// Secrets stored in the Secure Enclave Processor
    Enclave,
}

#[cfg(feature = "ffi")]
/// Converts the secret key context to an id that can be passed across the FFI boundary
unsafe impl IntoFfi for SecretKeyContext {
    type Value = u64;

    fn ffi_default() -> Self::Value {
        0
    }

    fn into_ffi_value(self) -> Self::Value {
        match self {
            SecretKeyContext::Memory(id) => id as u64,
            _ => Self::ffi_default(),
        }
    }
}

/// Represents specific secrets employable by the vault
#[derive(Clone, Debug)]
pub enum SecretKey {
    /// Raw buffer of bytes
    Buffer(Vec<u8>),
    /// AES-128 bit key
    Aes128([u8; 16]),
    /// AES-256 bit key
    Aes256([u8; 32]),
    /// x25519 secret key
    Curve25519([u8; 32]),
    /// NIST P-256 (secp256r1, prime256v1) secret key
    P256([u8; 32]),
}

impl SecretKey {
    /// Create a new Secret key using `data` and of the correct type
    pub fn new<B: AsRef<[u8]>>(data: B, xtype: SecretKeyType) -> Self {
        match xtype {
            SecretKeyType::Buffer(l) => SecretKey::Buffer(data.as_ref()[..l].to_vec()),
            SecretKeyType::Aes128 => SecretKey::Aes128(*array_ref![data.as_ref(), 0, 16]),
            SecretKeyType::Aes256 => SecretKey::Aes256(*array_ref![data.as_ref(), 0, 32]),
            SecretKeyType::P256 => SecretKey::P256(*array_ref![data.as_ref(), 0, 32]),
            SecretKeyType::Curve25519 => SecretKey::Curve25519(*array_ref![data.as_ref(), 0, 32]),
        }
    }
}

impl AsRef<[u8]> for SecretKey {
    fn as_ref(&self) -> &[u8] {
        match self {
            SecretKey::Buffer(a) => a.as_slice(),
            SecretKey::Aes128(a) => a.as_ref(),
            SecretKey::Aes256(a) => a.as_ref(),
            SecretKey::Curve25519(a) => a.as_ref(),
            SecretKey::P256(a) => a.as_ref(),
        }
    }
}

impl PartialEq for SecretKey {
    fn eq(&self, other: &Self) -> bool {
        use SecretKey::*;
        match (self, other) {
            (Buffer(a), Buffer(b)) => a.as_slice().ct_eq(b.as_slice()).unwrap_u8() == 1u8,
            (Aes128(a), Aes128(b)) => a.as_ref().ct_eq(b.as_ref()).unwrap_u8() == 1u8,
            (Aes256(a), Aes256(b)) => a.as_ref().ct_eq(b.as_ref()).unwrap_u8() == 1u8,
            (Curve25519(a), Curve25519(b)) => a.as_ref().ct_eq(b.as_ref()).unwrap_u8() == 1u8,
            (P256(a), P256(b)) => a.as_ref().ct_eq(b.as_ref()).unwrap_u8() == 1u8,
            (_, _) => false,
        }
    }
}

impl Eq for SecretKey {}

impl Zeroize for SecretKey {
    fn zeroize(&mut self) {
        use SecretKey::*;
        match self {
            Buffer(ref mut a) => a.zeroize(),
            Aes128(ref mut a) => a.zeroize(),
            Aes256(ref mut a) => a.zeroize(),
            Curve25519(ref mut a) => a.zeroize(),
            P256(ref mut a) => a.zeroize(),
        }
    }
}

zdrop_impl!(SecretKey);

/// The supported public keys
#[derive(Copy, Clone)]
pub enum PublicKey {
    /// x25519 Public Key
    Curve25519([u8; 32]),
    /// NIST P-256 (secp256r1, prime256v1) uncompressed public key
    P256([u8; 65]),
}

impl PublicKey {
    fn print(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        use PublicKey::*;
        match self {
            Curve25519(a) => write!(f, "PublicKey::Curve25519 {{ {} }}", hex::encode(a.as_ref())),
            P256(a) => write!(f, "PublicKey::P256 {{ {} }}", hex::encode(a.as_ref())),
        }
    }

    /// True if this is a Curve25519 Public Key
    pub fn is_curve25519(&self) -> bool {
        use PublicKey::*;
        matches!(self, Curve25519(..))
    }

    /// True if this is a NIST P-256 (secp256r1, prime256v1) uncompressed public key
    pub fn is_p256(&self) -> bool {
        use PublicKey::*;
        matches!(self, P256(..))
    }
}

impl AsRef<[u8]> for PublicKey {
    fn as_ref(&self) -> &[u8] {
        use PublicKey::*;
        match self {
            Curve25519(a) => a,
            P256(a) => a,
        }
    }
}

impl std::fmt::Display for PublicKey {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        self.print(f)
    }
}

impl std::fmt::Debug for PublicKey {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        self.print(f)
    }
}

impl PartialEq for PublicKey {
    fn eq(&self, other: &Self) -> bool {
        use PublicKey::*;
        match (self, other) {
            (Curve25519(a), Curve25519(b)) => a.ct_eq(b).unwrap_u8() == 1,
            (P256(a), P256(b)) => a.ct_eq(b).unwrap_u8() == 1,
            (_, _) => false,
        }
    }
}

impl Eq for PublicKey {}
