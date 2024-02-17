if true; then
  echo Outer condition is true

  if false; then
    echo Inner condition is true
  else
    echo Inner condition is false
  fi

else
  echo Outer condition is false
fi
