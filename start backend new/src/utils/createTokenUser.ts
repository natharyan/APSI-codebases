const createTokenUser = (user: any) => {
  return { name: user.name, userId: user._id, role: user.role }
}

export { createTokenUser }
