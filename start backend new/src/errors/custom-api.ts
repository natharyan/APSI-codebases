class CustomAPIError extends Error {
  static UnauthorizedError: any
  constructor(message: any) {
    super(message)
  }
}

export { CustomAPIError }